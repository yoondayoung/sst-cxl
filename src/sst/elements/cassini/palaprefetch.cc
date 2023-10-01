// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "palaprefetch.h"

#include <unordered_map>
#include <vector>
#include <deque>
#include <algorithm>
#include <fstream>

#include "stdlib.h"

#include "sst/core/params.h"

using namespace SST;
using namespace SST::Cassini;

void PalaPrefetcher::notifyAccess(const CacheListenerNotification& notify)
{
    const NotifyAccessType notifyType = notify.getAccessType();

    if (notifyType != READ && notifyType != WRITE)
        return;

    const NotifyResultType notifyResType = notify.getResultType();
    const Addr addr = notify.getPhysicalAddress();
    // if (notifyResType==MISS) return;
    // Insert address into the history table using the tag as the index
    Addr tag;
    if (prefetcherMode<3){ // 0: 기존, 1: only stride, 2: stride+(stride 발견 못하면)next block
        if(notifyResType != MISS) return;
        if(prefetcherMode==0) {
            tag = addr >> (addressSize - tagSize);
        }
        else { // ip based prefetcher
            tag = notify.getInstructionPointer();
        }
        StrideFilter filterEntry;
        filterEntry.lastAddress = addr;
        if (prefetcherMode==1) filterEntry.stride = -1;
        else filterEntry.stride = 0;
        filterEntry.lastStride = 0;
        filterEntry.state = P_INVALID;

        // If the value is already present, then we need to check its state information
        // and update the values in the table. If the stride values match for two addresses
        // in a row, then we update the stride value in the table. Otherwise, the value
        // remains unchanged.
        int32_t tempStride = 0;
        std::pair < std::unordered_map< uint64_t, StrideFilter >::iterator, bool >  retVal;
        retVal = recentAddrList->insert ( std::pair< uint64_t, StrideFilter >(tag, filterEntry) );
        if( retVal.second == false )
        {
            tempStride = int32_t( addr - (*recentAddrList)[tag].lastAddress );
            // (*recentAddrList)[tag].stride = tempStride;
        
            if( (*recentAddrList)[tag].state == P_INVALID )
            {
                if( (*recentAddrList)[tag].lastStride == tempStride )
                {
                    (*recentAddrList)[tag].stride = tempStride;
                    (*recentAddrList)[tag].state = P_VALID;
                }
            }
            else
            {
                if( (*recentAddrList)[tag].lastStride != tempStride )
                {
                    if (prefetcherMode==1) (*recentAddrList)[tag].stride = -1;
                    else (*recentAddrList)[tag].stride = 0;
                    (*recentAddrList)[tag].state = P_INVALID;
                }
            }

            (*recentAddrList)[tag].lastStride = tempStride;
            (*recentAddrList)[tag].lastAddress = addr;

            // Insert a reference to the new element at the front of the queue
            // and remove any other references to the element. Requires traversal
            // of the list (max n - 1), but need to keep it lru.
            if( recentAddrListQueue->front() != retVal.first )
            {
                for( it = recentAddrListQueue->begin(); it != recentAddrListQueue->end(); it++ )
                {
                    if( *it == retVal.first )
                    {
                        recentAddrListQueue->erase(it);
                        recentAddrListQueue->push_front(retVal.first);
                        break;
                    }
                }
            }
        }
        else
        {
            // Insert a reference to the new element at the front of the queue
            recentAddrListQueue->push_front(retVal.first);
        }

        if( recentAddrList->size() >= recentAddrListCount )
        {
            recentAddrList->erase(recentAddrListQueue->back());
            recentAddrListQueue->pop_back();
        }
        recheckCountdown = (recheckCountdown + 1) % strideDetectionRange;

        notifyResType == MISS ? missEventsProcessed++ : hitEventsProcessed++;

        if(recheckCountdown == 0 && notifyResType==MISS)
            DispatchRequest(addr, tag);
    }
    else if(prefetcherMode==3){ // oracle like
        // 들어온 ip & addr로 다음 addr 찾기
        bool prefetchissue = true;
        tag = notify.getInstructionPointer();
        if (DNNPrefetchTable->count(tag)==0) prefetchissue = false;
        recheckCountdown = (recheckCountdown + 1) % strideDetectionRange;

        notifyResType == MISS ? missEventsProcessed++ : hitEventsProcessed++;

        if(recheckCountdown == 0 && prefetchissue==true)
            DispatchRequest(addr, tag);
    }
    else if(prefetcherMode==4){ //only in prefetch allow pc
        tag = notify.getInstructionPointer();
        // not in prefetch list -> prefetch drop
        if (find(prefetchIPList->begin(), prefetchIPList->end(), tag)==prefetchIPList->end()) return;
        StrideFilter filterEntry;
        filterEntry.lastAddress = addr;
        // filterEntry.stride = blockSize;
        filterEntry.stride = -1;
        filterEntry.lastStride = 0;
        filterEntry.state = P_INVALID;

        // If the value is already present, then we need to check its state information
        // and update the values in the table. If the stride values match for two addresses
        // in a row, then we update the stride value in the table. Otherwise, the value
        // remains unchanged.
        int32_t tempStride = 0;
        std::pair < std::unordered_map< uint64_t, StrideFilter >::iterator, bool >  retVal;
        retVal = recentAddrList->insert ( std::pair< uint64_t, StrideFilter >(tag, filterEntry) );
        if( retVal.second == false )
        {
            tempStride = int32_t( addr - (*recentAddrList)[tag].lastAddress );
            if( (*recentAddrList)[tag].state == P_INVALID )
            {
                if( (*recentAddrList)[tag].lastStride == tempStride )
                {
                    (*recentAddrList)[tag].state = P_PENDING;
                }
            }
            else if( (*recentAddrList)[tag].state == P_PENDING )
            {
                if( (*recentAddrList)[tag].lastStride == tempStride )
                {
                    (*recentAddrList)[tag].state = P_VALID;
                    (*recentAddrList)[tag].stride = tempStride;
                }

            }
            else
            {
                if( (*recentAddrList)[tag].lastStride != tempStride )
                {
                    (*recentAddrList)[tag].state = P_PENDING;
                }
            }

            (*recentAddrList)[tag].lastStride = tempStride;
            (*recentAddrList)[tag].lastAddress = addr;

            // Insert a reference to the new element at the front of the queue
            // and remove any other references to the element. Requires traversal
            // of the list (max n - 1), but need to keep it lru.
            if( recentAddrListQueue->front() != retVal.first )
            {
                for( it = recentAddrListQueue->begin(); it != recentAddrListQueue->end(); it++ )
                {
                    if( *it == retVal.first )
                    {
                        recentAddrListQueue->erase(it);
                        recentAddrListQueue->push_front(retVal.first);
                        break;
                    }
                }
            }
        }
        else
        {
            // Insert a reference to the new element at the front of the queue
            recentAddrListQueue->push_front(retVal.first);
        }

        if( recentAddrList->size() >= recentAddrListCount )
        {
            recentAddrList->erase(recentAddrListQueue->back());
            recentAddrListQueue->pop_back();
        }
        recheckCountdown = (recheckCountdown + 1) % strideDetectionRange;

        notifyResType == MISS ? missEventsProcessed++ : hitEventsProcessed++;

        if(recheckCountdown == 0)
            DispatchRequest(addr, tag);
    }
    else{
        std::cout << "prefetcherMode error!" << std::endl;
    }
    
}

void PalaPrefetcher::DispatchRequest(Addr targetAddress, Addr tag)
{
    /*  TODO Needs to be updated with current MemHierarchy Commands/States, MemHierarchyInterface */
    MemEvent* ev = NULL;
    Addr targetPrefetchAddress;
    int32_t stride;

    if(prefetcherMode<3 || prefetcherMode==4){
        // uint64_t tag = targetAddress >> (addressSize - tagSize);
        stride = (*recentAddrList)[tag].stride;
        if(stride==-1) return;
        else if(stride==0) stride = blockSize;
        targetPrefetchAddress = targetAddress + (strideReach * stride);
    }
    else if(prefetcherMode==3){ 
        uint32_t max_index_len = (*DNNPrefetchTable)[tag].size()-1;
        uint32_t index = find((*DNNPrefetchTable)[tag].begin(), (*DNNPrefetchTable)[tag].end(), targetAddress)-(*DNNPrefetchTable)[tag].begin();
        // 만약 이게 마지막 인덱스거나 tag가 table안에 없으면 drop
        if (index>=max_index_len) {
            stride = blockSize;
            targetPrefetchAddress = targetAddress + (strideReach * stride);
        }
        // 아니라면 prefetch 진행
        else{
            index = min(index+strideReach, max_index_len);
            targetPrefetchAddress = (*DNNPrefetchTable)[tag][index];
            stride = targetPrefetchAddress - targetAddress;
        }
    }
    targetPrefetchAddress = targetPrefetchAddress - (targetPrefetchAddress % blockSize);

    if(overrunPageBoundary)
    {
        // output->verbose(CALL_INFO, 2, 0,
        //         "Issue prefetch, target address: %" PRIx64 ", prefetch address: %" PRIx64 " (reach out: %" PRIu32 ", stride=%" PRIu32 "), prefetchAddress=%" PRIu64 "\n",
        //         targetAddress, targetAddress + (strideReach * stride),
        //         (strideReach * stride), stride, targetPrefetchAddress);

        statPrefetchOpportunities->addData(1);

        // Check next address is aligned to a cache line boundary
        assert(targetPrefetchAddress % blockSize == 0);

        ev = new MemEvent(getName(), targetPrefetchAddress, targetPrefetchAddress, Command::GetS);
    }
    else
    {
        const Addr targetAddressPhysPage = targetAddress / pageSize;
        const Addr targetPrefetchAddressPage = targetPrefetchAddress / pageSize;

        // Check next address is aligned to a cache line boundary
        assert(targetPrefetchAddress % blockSize == 0);

        // if the address we found and the next prefetch address are on the same
        // we can safely prefetch without causing a page fault, otherwise we
        // choose to not prefetch the address
        if(targetAddressPhysPage == targetPrefetchAddressPage)
        {
            output->verbose(CALL_INFO, 2, 0, "Issue prefetch, target address: %" PRIx64 ", prefetch address: %" PRIx64 " (reach out: %" PRIu32 ", stride=%" PRIu32 ")\n",
                        targetAddress, targetPrefetchAddress, (strideReach * stride), stride);
            ev = new MemEvent(getName(), targetPrefetchAddress, targetPrefetchAddress, Command::GetS);
            statPrefetchOpportunities->addData(1);
        }
        else
        {
            output->verbose(CALL_INFO, 2, 0, "Cancel prefetch issue, request exceeds physical page limit\n");
            output->verbose(CALL_INFO, 4, 0, "Target address: %" PRIx64 ", page=%" PRIx64 ", Prefetch address: %" PRIx64 ", page=%" PRIx64 "\n", targetAddress,
                            targetAddressPhysPage, targetPrefetchAddress, targetPrefetchAddressPage);

            statPrefetchIssueCanceledByPageBoundary->addData(1);
            ev = NULL;
        }
    }

    if(ev != NULL)
    {
        std::vector<Event::HandlerBase*>::iterator callbackItr;

        Addr prefetchCacheLineBase = ev->getAddr() - (ev->getAddr() % blockSize);
        bool inHistory = false;
        const uint32_t currentHistCount = prefetchHistory->size();

        output->verbose(CALL_INFO, 2, 0, "Checking prefetch history for cache line at base %" PRIx64 ", valid prefetch history entries=%" PRIu32 "\n", prefetchCacheLineBase,
                        currentHistCount);

        for(uint32_t i = 0; i < currentHistCount; ++i)
        {
            if(prefetchHistory->at(i) == prefetchCacheLineBase)
            {
                inHistory = true;
                break;
            }
        }

        if(! inHistory)
        {
            statPrefetchEventsIssued->addData(1);

            // Remove the oldest cache line
            if(currentHistCount == prefetchHistoryCount)
            {
                    prefetchHistory->pop_front();
            }

            // Put the cache line at the back of the queue
            prefetchHistory->push_back(prefetchCacheLineBase);

            assert((ev->getAddr() % blockSize) == 0);

            // Cycle over each registered call back and notify them that we want to issue a prefet$
            for(callbackItr = registeredCallbacks.begin(); callbackItr != registeredCallbacks.end(); callbackItr++)
            {
                // Create a new read request, we cannot issue a write because the data will get
                // overwritten and corrupt memory (even if we really do want to do a write)
                MemEvent* newEv = new MemEvent(getName(), ev->getAddr(), ev->getAddr(), Command::GetS);
                newEv->setSize(blockSize);
                newEv->setPrefetchFlag(true);

                (*(*callbackItr))(newEv);
            }

            delete ev;
        }
        else
        {
            statPrefetchIssueCanceledByHistory->addData(1);
            output->verbose(CALL_INFO, 2, 0, "Prefetch canceled - same cache line is found in the recent prefetch history.\n");
            delete ev;
        }
    }
}


PalaPrefetcher::PalaPrefetcher(ComponentId_t id, Params& params) : CacheListener(id, params)
{
    requireLibrary("memHierarchy");

    verbosity = params.find<int>("verbose", 0);

    char* new_prefix = (char*) malloc(sizeof(char) * 128);
    snprintf(new_prefix, sizeof(char)*128, "PalaPrefetcher[%s | @f:@p:@l] ", getName().c_str());
    output = new Output(new_prefix, verbosity, 0, Output::STDOUT);
    free(new_prefix);

    recheckCountdown = 0;
    blockSize = params.find<uint64_t>("cache_line_size", 64);
    tagSize = params.find<uint64_t>("tag_size", 48);
    addressSize = params.find<uint64_t>("addr_size", 64);

    prefetchHistoryCount = params.find<uint32_t>("history", 16);
    prefetchHistory = new std::deque<uint64_t>();

    strideReach = params.find<uint32_t>("reach", 2);
    strideDetectionRange = params.find<uint64_t>("detect_range", 4);
    recentAddrListCount = params.find<uint32_t>("address_count", 64);
    pageSize = params.find<uint64_t>("page_size", 4096);

    uint32_t overrunPB = params.find<uint32_t>("overrun_page_boundaries", 0);
    overrunPageBoundary = (overrunPB == 0) ? false : true;

    prefetcherMode = params.find<uint64_t>("prefetcher_mode", 2);

    if (prefetcherMode==3){ // dnn custom prefetch (oracle prefetch)
        prefetchTablePath = params.find<string>("prefetch_table_path", "");
        DNNPrefetchTable = new std::unordered_map< uint64_t, std::vector<uint64_t> >;
        ifstream inf;
        inf.open(prefetchTablePath);
        while(!inf.eof()){
            if (inf.eof()){
                break;
            }
            uint64_t ip, addr;
            uint32_t length;
            std::vector<uint64_t> addrs;
            inf >> ip >> length;
            for (int i=0; i<length; i++){
                inf >> addr;
                addrs.push_back(addr);
            }
            DNNPrefetchTable->insert(std::pair<uint64_t,std::vector<uint64_t>>(ip, addrs));
        }
        // for checking
        std::cout << "prefetch table file read done. Prefetch Table Size:" << DNNPrefetchTable->size()  << std::endl;

    }
    if (prefetcherMode==4){ // pc(instruction)에 따라 prefetch 할 지 선택
        prefetchTablePath = params.find<string>("prefetch_table_path", "");
        prefetchIPList = new std::vector<uint64_t>;
        ifstream inf;
        inf.open(prefetchTablePath);
        while(!inf.eof()){
            if (inf.eof()){
                break;
            }
            uint64_t ip;
            inf >> ip;
            prefetchIPList->push_back(ip);
        }
        // for checking
        std::cout << "Prefetch IP list Size:" << prefetchIPList->size()  << std::endl;
    }

    nextRecentAddressIndex = 0;
    recentAddrList = new std::unordered_map< uint64_t, StrideFilter >;
    recentAddrListQueue = new std::deque< std::unordered_map< uint64_t, StrideFilter >::iterator >;

    output->verbose(CALL_INFO, 1, 0, "PalaPrefetcher created, cache line: %" PRIu64 ", page size: %" PRIu64 "\n",
            blockSize, pageSize);

    missEventsProcessed = 0;
    hitEventsProcessed = 0;

    statPrefetchOpportunities = registerStatistic<uint64_t>("prefetch_opportunities");
    statPrefetchEventsIssued = registerStatistic<uint64_t>("prefetches_issued");
    statPrefetchIssueCanceledByPageBoundary = registerStatistic<uint64_t>("prefetches_canceled_by_page_boundary");
    statPrefetchIssueCanceledByHistory = registerStatistic<uint64_t>("prefetches_canceled_by_history");
    std::cout << "palal done!" << std::endl;
}

PalaPrefetcher::~PalaPrefetcher()
{
    delete prefetchHistory;
    delete recentAddrList;
    delete recentAddrListQueue;
    if (prefetcherMode==3) delete DNNPrefetchTable;
}

void PalaPrefetcher::registerResponseCallback(Event::HandlerBase* handler)
{
    registeredCallbacks.push_back(handler);
}

void PalaPrefetcher::printStats(Output &out)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This is loosely base on the work done by Palacharla and Kessler. The prefetcher maintains a table
/// indexed by a tag. The table contains the current stride value as well as the previous address and
/// previous stride value. The stride value is updated when the previous stride matches for two
/// fetches in a row. The default stride is the size of a cache line (initial value). The table can
/// hold a number of entries equal to address_count.
///
/// S. Palacharla and R. E. Kessler. 1994. Evaluating stream buffers as a secondary cache replacement.
/// In Proceedings of the 21st annual international symposium on Computer architecture (ISCA '94).
/// IEEE Computer Society Press, Los Alamitos, CA, USA, 24-33. DOI=http://dx.doi.org/10.1145/191995.192014
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
