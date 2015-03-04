// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cache.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <csignal>
#include <boost/variant.hpp>

#include "cacheController.h"
#include "memEvent.h"
#include "mshr.h"
#include "coherenceControllers.h"
#include "hash.h"


using namespace SST;
using namespace SST::MemHierarchy;


/**
 *  Determine whether an access will be a cache hit or not
 *  Cache hit if:
 *      Line is present in the cache and
 *      Line is in the correct coherence state for the request (BottomCC state is correct) and
 *      Line is not currently being invalidated (TopCC state is correct)
 *  @return int indicating cache hit (0) or miss (1=cold miss, 2=bottomCC miss, 3= topCC miss)
 */
int Cache::isCacheHit(MemEvent* _event, Command _cmd, Addr _baseAddr) {
    int lineIndex = cf_.cacheArray_->find(_baseAddr,false);
     
    if (isCacheMiss(lineIndex))                         return 1;
    CacheLine* cacheLine = getCacheLine(lineIndex);
    if (bottomCC_->isCoherenceMiss(_event,cacheLine))   {
        State st = cacheLine->getState();
        if (st == I) return 1;
        else return 2;
    }
    if (!(_event->isPrefetch() && (_event->getRqstr() == this->getName())) && topCC_->isCoherenceMiss(_event,cacheLine))  return 3;
    
    return 0;
}

/*
 *  _mshrHit 
 *  true for: selfEvent that is a prefetch, re-activated event
 *  false for anything called through processIncomingEvent
 *
 *  Controls:
 *      Whether replacement state updated (update if false)
 *      Latency of access
 *
 */
/**
 *  Handle a request from upper level caches
 *  This function is only called if there are no blocking requests already in the MSHRs
 *  1. Locate line, if present, in the cache array & update its replacement manager state
 *  2. If cache miss, allocate a new cache line, possibly evicting another line
 *  3. If neccessary, get block/coherence permission from lower level caches/memory
 *  4. If neccessary, invalidate upper level caches and/or request updated data
 *  5. Send response to requestor
 *  throw exception if request cannot be completed immediately (cache miss) and place request in MSHRs 
 *  throw exception if request can be handled even if the line is in transition (i.e., PutS) 
 */
void Cache::processCacheRequest(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine;
    
    try{


        bool updateLine = !_mshrHit && MemEvent::isDataRequest(_cmd);   /* TODO: move replacement manager update to time when cache actually sends a response */
        int lineIndex = cf_.cacheArray_->find(_baseAddr, updateLine);   /* Update cacheline only if it's NOT mshrHit */
        
        if(isCacheMiss(lineIndex)){                                     /* Miss.  If needed, evict candidate */
            d_->debug(_L3_,"-- Cache Miss --\n");
            allocateCacheLine(_event, _baseAddr, lineIndex);            /* Function may except here to wait for eviction */
        }
        
        cacheLine = getCacheLine(lineIndex);
        bottomCC_->handleRequest(_event, cacheLine, _cmd, _mshrHit);    /* upgrade or fetch line from lower level caches */
        stallIfUpgradeInProgress(cacheLine);                            /* Stall if upgrade in progress */
        
        if(!(_event->isPrefetch() && (_event->getRqstr() == this->getName()))) { /* Don't send responses (through TopCC) if it is a prefetch request generated by this cache */
            bool done = topCC_->handleRequest(_event, cacheLine, _mshrHit);  /* Invalidate sharers, send respond to requestor if needed */
            postRequestProcessing(_event, cacheLine, done, _mshrHit);

        } else {
            recordLatency(_event);
        }
    }
    catch(blockedEventException const& e){

        processRequestInMSHR(_baseAddr, _event);                        /* This request needs to stall until another pending request finishes.  This event is now in the  MSHR waiting to 'reactive' upon completion of the outstanding request in progress  */
    }
    catch(ignoreEventException const& e){}
}

void Cache::processCacheReplacement(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine;
    
    try{
        bool updateLine = false;
        int lineIndex = cf_.cacheArray_->find(_baseAddr, updateLine);
        
        if(isCacheMiss(lineIndex)){                                     /* Miss.  If needed, evict candidate */
            d_->debug(_L3_,"-- Cache Miss --\n");
            checkCacheMissValidity(_event);
            allocateCacheLine(_event, _baseAddr, lineIndex);
        }
        
        cacheLine = getCacheLine(lineIndex);
        handleIgnorableRequests(_event, cacheLine, _cmd);               /* If a PutS, handle immediately even if the cache line is in transition */
        
        bottomCC_->handleRequest(_event, cacheLine, _cmd, _mshrHit);    /* update cache line with new data */
        
        bool done = topCC_->handleRequest(_event, cacheLine, _mshrHit);  /* Update sharer/owner state */
        postRequestProcessing(_event, cacheLine, done, _mshrHit);
    }
    catch(blockedEventException const& e){
        processRequestInMSHR(_baseAddr, _event);                        /* Request needs to stall until another pending request finishes.  Buffer in MSHR  */
    }
    catch(ignoreEventException const& e){}
}


void Cache::processCacheInvalidate(MemEvent* _event, Command _cmd, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    
    if (!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;
    int lineIndex = cacheLine->getIndex();

    if(!L1_ && !processInvRequestInMSHR(_baseAddr, _event)) {
        return;
    }
    topCC_->handleInvalidate(lineIndex, _event, _event->getRqstr(), _cmd, _mshrHit); /* Invalidate upper levels */
    if(invalidatesInProgress(lineIndex)) return;
    
    bottomCC_->handleInvalidate(_event, cacheLine, _cmd);               /* Invalidate this cache line */
    if(!L1_) mshr_->removeElement(_baseAddr, _event);
    delete _event;
}



void Cache::processCacheResponse(MemEvent* _responseEvent, Addr _baseAddr){
    CacheLine* cacheLine = getCacheLine(_baseAddr); assert(cacheLine);
    
    MemEvent* origRequest = getOrigReq(mshr_->lookup(_baseAddr));
    
    bottomCC_->handleResponse(_responseEvent, cacheLine, origRequest);
    activatePrevEvents(_baseAddr);

    delete _responseEvent;
}



void Cache::processFetch(MemEvent* _event, Addr _baseAddr, bool _mshrHit){
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    Command cmd = _event->getCmd();

    if(!shouldInvRequestProceed(_event, cacheLine, _baseAddr, _mshrHit)) return;

    int lineIndex = cacheLine->getIndex();

    /* L1s wont stall because they don't have any sharers */
    if(!L1_ && !processInvRequestInMSHR(_baseAddr, _event)) return;

    topCC_->handleInvalidate(lineIndex, _event, _event->getRqstr(), cmd, _mshrHit);
    if(invalidatesInProgress(lineIndex)) return;
    
    bottomCC_->handleFetchInvalidate(_event, cacheLine, cmd, _mshrHit);
    if(!L1_) mshr_->removeElement(_baseAddr, _event);

    delete _event;
}


void Cache::processFetchResp(MemEvent * _event, Addr _baseAddr) {
    CacheLine* cacheLine = getCacheLine(_baseAddr);
    int lineIndex = cacheLine->getIndex();

    topCC_->handleFetchResp(_event, cacheLine);     // update sharer/owner state
    bottomCC_->handleFetchResp(_event, cacheLine);  // update data
    activatePrevEvents(_baseAddr);

    delete _event;
}

/* ---------------------------------
   Writeback Related Functions
   --------------------------------- */

void Cache::allocateCacheLine(MemEvent* _event, Addr _baseAddr, int& _newCacheLineIndex) throw(blockedEventException){
    CacheLine* wbCacheLine = findReplacementCacheLine(_baseAddr);
    
    /* Is writeback candidate invalid and not in transition? */
    if(wbCacheLine->valid()){
        candidacyCheck(_event, wbCacheLine, _baseAddr);
        evictInHigherLevelCaches(wbCacheLine, _baseAddr);
        writebackToLowerLevelCaches(_event, wbCacheLine);
    }
    
    /* OK to change addr of topCC cache line, OK to replace cache line  */
    replaceCacheLine(wbCacheLine->getIndex(), _newCacheLineIndex, _baseAddr);    
    assert(!mshr_->exists(_baseAddr));
}



CacheArray::CacheLine* Cache::findReplacementCacheLine(Addr _baseAddr){
    int wbLineIndex = cf_.cacheArray_->preReplace(_baseAddr);
    CacheLine* wbCacheLine = cf_.cacheArray_->lines_[wbLineIndex];
    return wbCacheLine;
}



void Cache::candidacyCheck(MemEvent* _event, CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(blockedEventException){
    d_->debug(_L4_,"Evicting 0x%" PRIx64 ", St: %s\n", _wbCacheLine->getBaseAddr(), BccLineString[_wbCacheLine->getState()]);
    
    if(_wbCacheLine->isLocked()){
        d_->debug(_L8_, "Warning: Replacement cache line is user-locked. WbCLine Addr: %" PRIx64 "\n", _wbCacheLine->getBaseAddr());
        _wbCacheLine->setEventsWaitingForLock(true);
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw blockedEventException();
    }
    else if(isCandidateInTransition(_wbCacheLine)){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw blockedEventException();
    }
    return;
}



bool Cache::isCandidateInTransition(CacheLine* _wbCacheLine){
    CCLine* wbCCLine = topCC_->getCCLine(_wbCacheLine->getIndex());
    if(wbCCLine->inTransition() || CacheLine::inTransition(_wbCacheLine->getState())){
        d_->debug(_L3_,"Stalling request: Replacement cache line in transition.\n");
        return true;
    }
    return false;
}



void Cache::evictInHigherLevelCaches(CacheLine* _wbCacheLine, Addr _requestBaseAddr) throw(blockedEventException){
    topCC_->handleEviction(_wbCacheLine->getIndex(), this->getName(), _wbCacheLine->getState());
    CCLine* ccLine = topCC_->getCCLine(_wbCacheLine->getIndex());
    
    if(ccLine->inTransition()){
        mshr_->insertPointer(_wbCacheLine->getBaseAddr(), _requestBaseAddr);
        throw blockedEventException();
    }
    
    ccLine->clear();
}



void Cache::writebackToLowerLevelCaches(MemEvent* _event, CacheLine* _wbCacheLine){
    bottomCC_->handleEviction(_wbCacheLine, _event->getGroupId(), this->getName());
}



void Cache::replaceCacheLine(int _replacementCacheLineIndex, int& _newCacheLineIndex, Addr _newBaseAddr){
    CCLine* wbCCLine = topCC_->getCCLine(_replacementCacheLineIndex);
    wbCCLine->clear();
    wbCCLine->setBaseAddr(_newBaseAddr);

    _newCacheLineIndex = _replacementCacheLineIndex;
    cf_.cacheArray_->replace(_newBaseAddr, _newCacheLineIndex);
}


/* ---------------------------------------
   Invalidate Request helper functions
   --------------------------------------- */
bool Cache::invalidatesInProgress(int _lineIndex){
    if(L1_) return false;
    
    CCLine* ccLine = topCC_->getCCLine(_lineIndex);
    if(ccLine->inTransition()){
        d_->debug(_L7_,"Invalidate request forwared to HiLv caches.\n");
        return true;
    }
    return false;
}



/* Multiple 'returns' in the function used for performance (as opposed to single return) */
bool Cache::shouldInvRequestProceed(MemEvent* _event, CacheLine* _cacheLine, Addr _baseAddr, bool _mshrHit){
    /* Scenario where this 'if' occurs:  HiLv$ evicts a shared line (S->I), sends PutS to LowLv$.
       Simultaneously, LowLv$ sends an Inv to HiLv$. Thus, HiLv$ sends an Inv an already invalidated line */
    if(!_cacheLine || (_cacheLine->getState() == I && !_mshrHit)){
        d_->debug(_WARNING_,"Ignoring Request: Cache Line doesn't exist or invalid.\n");
        return false;
    }

    if(_cacheLine->isLocked()){                                 /* If user-locked then wait this lock is released to activate this event. */
        if(!processInvRequestInMSHR(_baseAddr, _event)) {
            return false;
        }
        incInvalidateWaitingForUserLock(groupId);               /* Requests is in MSHR.  Stall and wait for the atomic modet to be 'cleared' */
        _cacheLine->setEventsWaitingForLock(true);
        d_->debug(_L8_,"Stalling request:  Cache line is in atomic mode.\n");
        
        return false;
    }
    
    CCLine* ccLine = topCC_->getCCLine(_cacheLine->getIndex());
    if(ccLine->getState() != V){                                /* Check if invalidates are already in progress (A writeback is going on?) */
        processInvRequestInMSHR(_baseAddr, _event);                /* Whether a NACK was sent or not, request needs to stall */
        return false;
    }
    
    return true;
}

/* -------------------------------------------------------------------------------------
            Helper Functions
 ------------------------------------------------------------------------------------- */


void Cache::stallIfUpgradeInProgress(CacheLine* _cacheLine) throw(blockedEventException){
    if(_cacheLine->inTransition()){
        throw blockedEventException();                          /* stall request if upgrade is in progress */
    }
}



void Cache::activatePrevEvents(Addr _baseAddr){
    if(!mshr_->isHit(_baseAddr)) return;
    
    vector<mshrType> mshrEntry = mshr_->removeAll(_baseAddr);
    bool cont;
    int i = 0;
    d_->debug(_L3_,"---------Replaying Events--------- Size: %lu\n", mshrEntry.size());
    
    for(vector<mshrType>::iterator it = mshrEntry.begin(); it != mshrEntry.end(); i++){
        if((*it).elem.type() == typeid(Addr)){                          /* Pointer Type */
            Addr pointerAddr = boost::get<Addr>((*it).elem);
            d_->debug(_L6_,"Pointer Addr: %" PRIx64 "\n", pointerAddr);
            if(!mshr_->isHit(pointerAddr)){                             /* Entry has been already been processed, delete mshr entry */
                mshrEntry.erase(it);
                continue;
            }
            
            vector<mshrType> pointerMSHR = mshr_->removeAll(pointerAddr);

            for(vector<mshrType>::iterator it2 = pointerMSHR.begin(); it2 != pointerMSHR.end(); i++){
                assert((*it2).elem.type() == typeid(MemEvent*));
                cont = activatePrevEvent(boost::get<MemEvent*>((*it2).elem), pointerMSHR, pointerAddr, it2, i);
                if(!cont) break;
            }
            
            mshrEntry.erase(it);
            if(mshr_->isHit(_baseAddr)){
                mshr_->insertAll(_baseAddr, mshrEntry);
                break;
            }
        }
        else{                                                           /* MemEvent Type */
            SimTime_t start = boost::get<MemEvent*>((*it).elem)->getStartTime();
            cont = activatePrevEvent(boost::get<MemEvent*>((*it).elem), mshrEntry, _baseAddr, it, i);
            if(!cont) break;
            else{
                /* only update upgrade latency on first replayed event. Other "MSHR hits" 
                   are not really upgrades, they are just blocked events */
                if(i == 0) updateUpgradeLatencyAverage(start);
            }
        }
    }
    d_->debug(_L3_,"---------end---------\n");
}



bool Cache::activatePrevEvent(MemEvent* _event, vector<mshrType>& _mshrEntry, Addr _addr, vector<mshrType>::iterator _it, int _i){
    d_->debug(_L3_,"Replaying event #%i, cmd = %s, bsAddr: %" PRIx64 ", addr: %" PRIx64 ", dst: %s\n",
                  _i, CommandString[_event->getCmd()], toBaseAddr(_event->getAddr()), _event->getAddr(), _event->getDst().c_str());
    d_->debug(_L3_,"--------------------------------------\n");
    
    this->processEvent(_event, true);
    
    d_->debug(_L3_,"--------------------------------------\n");
    _mshrEntry.erase(_it);
    
    /* If the event we just ran 'blocked', then there is not reason to activate other events. */
    if(mshr_->isHit(_addr)){
        mshr_->insertAll(_addr, _mshrEntry);
        return false;
    }
    return true;
}

void Cache::recordLatency(MemEvent* _event) {
    uint64 t = (startTimeList.find(_event))->second;
    if (missTypeList.find(_event) != missTypeList.end()) {
        int missType = missTypeList.find(_event)->second;
        switch (missType) {
            case 0:
                missLatency_GetS_IS += (timestamp_ - t + 1);
                break;
            case 1:
                missLatency_GetS_M += (timestamp_ - t + 1);
                break;
            case 2:
                missLatency_GetX_IM += (timestamp_ - t + 1);
                break;
            case 3:
                missLatency_GetX_SM += (timestamp_ - t + 1);
                break;
            case 4:
                missLatency_GetX_M += (timestamp_ - t + 1);
                break;
            case 5:
                missLatency_GetSEx_IM += (timestamp_ - t + 1);
                break;
            case 6:
                missLatency_GetSEx_SM += (timestamp_ - t + 1);
                break;
            case 7:
                missLatency_GetSEx_M += (timestamp_ - t + 1);
                break;
            default:
                break;
        }
        missTypeList.erase(_event);
    }
    startTimeList.erase(_event);
}


void Cache::postRequestProcessing(MemEvent* _event, CacheLine* _cacheLine, bool _requestCompleted, bool _mshrHit) throw(blockedEventException){
    Command cmd    = _event->getCmd();
    Addr baseAddr  = _cacheLine->getBaseAddr();
    CCLine* ccLine = topCC_->getCCLine(_cacheLine->getIndex());
    
    if(_requestCompleted){
        if(cmd != PutS && !(_cacheLine->inStableState() && ccLine->inStableState())) {    /* Sanity check cache state */
            d_->fatal(CALL_INFO, -1, "Error: Finished handling request but cache line is not stable: cmd = %s, Bcc state = %s, Tcc state = %s\n",
                    CommandString[cmd],BccLineString[_cacheLine->getState()],TccLineString[ccLine->getState()]);
        }
        /* Upon a PutS (due to invalidate, ie mshrEntry exists), only possible pending request should be a GetSEx request, make sure this is the case */
        if(cmd == PutS && mshr_->exists(baseAddr)){
            assert(getOrigReq(mshr_->lookup(baseAddr))->getCmd() == GetSEx);
        }
        /* MemHierarchy models a "blocking cache", it is important to 'replay' blocked
           events that were waiting for this event to complete */
        if(!_mshrHit && MemEvent::isWriteback(cmd)) activatePrevEvents(baseAddr);
        
        /* For atomic requests handled by the cache itself, GetX unlocks the cache line.  Therefore,
           we possibly need to 'replay' events that blocked due to an locked cacheline */
        reActivateEventWaitingForUserLock(_cacheLine);
        // record time for this request and remove it
        recordLatency(_event);
        delete _event;
    }
    else if(cmd != PutS) throw blockedEventException();  
}


void Cache::reActivateEventWaitingForUserLock(CacheLine* _cacheLine){
    Addr baseAddr = _cacheLine->getBaseAddr();
    if(_cacheLine->getEventsWaitingForLock() && !_cacheLine->isLocked()){
        _cacheLine->setEventsWaitingForLock(false);
        if(mshr_->isHit(baseAddr)) activatePrevEvents(baseAddr);
    }
}



void Cache::handleIgnorableRequests(MemEvent* _event, CacheLine* _cacheLine, Command _cmd) throw(ignoreEventException){
    /* If cache line is in transition, that means this requests is a writeback from a lower level cache.
       In this case, it has to be a PutS requests because the only possible transition going on is SM.  We can just ignore
       the request after removing the sharer. */
    if(_cacheLine->inTransition()){
        assert(_cmd == PutS);
        topCC_->handleRequest(_event, _cacheLine, false);
        d_->debug(_L3_,"Sharer removed while cache line was in transition. Cmd = %s, St = %s\n", CommandString[_cmd], BccLineString[_cacheLine->getState()]);
        throw ignoreEventException();
    }
}



bool Cache::isCacheMiss(int _lineIndex){
    if(_lineIndex == -1){
        return true;
    }
    else return false;
}


/* ---------------------------------------
   Extras
   --------------------------------------- */
MemEvent* Cache::getOrigReq(const vector<mshrType> _mshrEntry){
    assert(_mshrEntry.front().elem.type() == typeid(MemEvent*));

    return boost::get<MemEvent*>(_mshrEntry.front().elem);
}



void Cache::updateUpgradeLatencyAverage(SimTime_t start){
    uint64_t latency = timestamp_ - start;
    d_->debug(_INFO_,"Latency = %" PRIu64 "\n", latency);
    totalUpgradeLatency_ += latency;
    upgradeCount_++;
}


void Cache::errorChecking(){    
    if(cf_.MSHRSize_ <= 0)             _abort(Cache, "MSHR size not specified correctly. \n");
    if(cf_.numLines_ <= 0)             _abort(Cache, "Number of lines not set correctly. \n");
    if(!isPowerOfTwo(cf_.lineSize_))   _abort(Cache, "Cache line size is not a power of two. \n");
}



void Cache::pMembers(){
    string protocolStr;
    if(cf_.protocol_) protocolStr = "MESI";
    else protocolStr = "MSI";
    
    d_->debug(_INFO_,"Coherence Protocol: %s \n", protocolStr.c_str());
    d_->debug(_INFO_,"Cache lines: %d \n", cf_.numLines_);
    d_->debug(_INFO_,"Cache line size: %d \n", cf_.lineSize_);
    d_->debug(_INFO_,"MSHR entries:  %d \n\n", cf_.MSHRSize_);
}



void Cache::retryRequestLater(MemEvent* _event){
    retryQueueNext_.push_back(_event);
}



CacheArray::CacheLine* Cache::getCacheLine(Addr _baseAddr){
    int lineIndex =  cf_.cacheArray_->find(_baseAddr, false);
    if(lineIndex == -1) return NULL;
    else return cf_.cacheArray_->lines_[lineIndex];
}



CacheArray::CacheLine* Cache::getCacheLine(int _lineIndex){
    if(_lineIndex == -1) return NULL;
    else return cf_.cacheArray_->lines_[_lineIndex];
}



bool Cache::processRequestInMSHR(Addr _baseAddr, MemEvent* _event){
    if (mshr_->insert(_baseAddr, _event)) {
        _event->setStartTime(timestamp_);
        return true;
    } else {
        sendNACK(_event);
        return false;
    }
}


/* Invalidations/fetches will wait for the current outstanding transaction, but no waiting ones! */
bool Cache::processInvRequestInMSHR(Addr _baseAddr, MemEvent* _event){
    if (mshr_->insertInv(_baseAddr, _event)) {
        _event->setStartTime(timestamp_);
        return true;
    } else {
        sendNACK(_event);
        return false;
    }
}


void Cache::sendNACK(MemEvent* _event){
    if(_event->isHighNetEvent())        topCC_->sendNACK(_event);
    else if(_event->isLowNetEvent())    bottomCC_->sendNACK(_event);
    else
        _abort(Cache, "Command type not recognized, Cmd = %s\n", CommandString[_event->getCmd()]);
}


/*
 *  Response latency: MSHR latency because MSHR lookup to find event that was nacked. No cache access.
 */
void Cache::processIncomingNACK(MemEvent* _origReqEvent){
    d_->debug(_L3_,"NACK received.\n");

    /* Determine what CC will retry sending the event */
    if(_origReqEvent->fromHighNetNACK())       topCC_->resendEvent(_origReqEvent);
    else if(_origReqEvent->fromLowNetNACK())   bottomCC_->resendEvent(_origReqEvent);
    else
        _abort(Cache, "Command type not recognized, Cmd = %s\n", CommandString[_origReqEvent->getCmd()]);
    
}



void Cache::checkCacheMissValidity(MemEvent* _event) throw(ignoreEventException){
    Command cmd = _event->getCmd();
    assert(cmd != PutM && cmd != PutE && cmd != PutX && cmd != PutXE);
    if(cmd == PutS) throw ignoreEventException();
}



bool operator== ( const mshrType& _n1, const mshrType& _n2) {
    if(_n1.elem.type() == typeid(Addr)) return false;
    return(boost::get<MemEvent*>(_n1.elem) == boost::get<MemEvent*>(_n2.elem));
}



void Cache::incTotalRequestsReceived(int _groupId){
    stats_[0].TotalRequestsReceived_++;
    if(groupStats_){
        stats_[_groupId].TotalRequestsReceived_++;
    }
}



void Cache::incTotalMSHRHits(int _groupId){
    stats_[0].TotalMSHRHits_++;
    if(groupStats_){
        stats_[_groupId].TotalMSHRHits_++;
    }
}

void Cache::incInvalidateWaitingForUserLock(int _groupId){
    stats_[0].InvWaitingForUserLock_++;
    if(groupStats_){
        stats_[_groupId].InvWaitingForUserLock_++;
    }
}

//HOOK'em Horns!

