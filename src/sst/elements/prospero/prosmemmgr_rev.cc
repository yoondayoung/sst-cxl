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
#include "prosmemmgr.h"

using namespace SST::Prospero;

ProsperoMemoryManager::ProsperoMemoryManager(const uint64_t pgSize, Output* out, const uint64_t actmemSize) :
	pageSize(pgSize) {

	output = out;
	nextPageStart_a = 0;
	nextPageStart = actmemSize;
}

ProsperoMemoryManager::~ProsperoMemoryManager() {

}

uint64_t ProsperoMemoryManager::translate(const uint64_t virtAddr) {
	const uint64_t pageOffset = virtAddr % pageSize;
	const uint64_t virtPageStart = virtAddr - pageOffset;
	uint64_t resolvedPhysPageStart = 0;

	output->verbose(CALL_INFO, 2, 0, "Translating virtual address %" PRIu64 ", page offset=s%" PRIu64 ", start virt=%" PRIu64 "\n",
		virtAddr, pageOffset, virtPageStart);

	std::map<uint64_t, uint64_t>::iterator findEntry = pageTable.find(virtPageStart);
	if(findEntry == pageTable.end()) {
		output->verbose(CALL_INFO, 2, 0, "Translation requires new page, creating at physical: %" PRIu64 "\n", nextPageStart);

		resolvedPhysPageStart = nextPageStart;
		pageTable.insert( std::pair<uint64_t, uint64_t>(virtPageStart, nextPageStart) );
		nextPageStart += pageSize;
	} else {
		resolvedPhysPageStart = findEntry->second;
	}

	output->verbose(CALL_INFO, 2, 0, "Translated physical page to %" PRIu64 " + offset %" PRIu64 " = final physical %" PRIu64 "\n",
		resolvedPhysPageStart, pageOffset, (resolvedPhysPageStart + pageOffset));

	// Reapply the offset to the physical page we just located and we are finished
	return resolvedPhysPageStart + pageOffset;
}

// activation data addr translation 
// activation 의 경우 nextPageStart가 actmemSize를 넘는다면, 다시 0으로 돌리고 pageTable에서 0인 key값을 찾아서 해당 element를 삭제해야 함.
uint64_t ProsperoMemoryManager::translate_ACT(const uint64_t virtAddr) {
	const uint64_t pageOffset = virtAddr % pageSize;
	const uint64_t virtPageStart = virtAddr - pageOffset;
	uint64_t resolvedPhysPageStart = 0;

	output->verbose(CALL_INFO, 2, 0, "Translating virtual address %" PRIu64 ", page offset=s%" PRIu64 ", start virt=%" PRIu64 "\n",
		virtAddr, pageOffset, virtPageStart);

	std::map<uint64_t, uint64_t>::iterator findEntry = pageTable.find(virtPageStart);
	if(findEntry == pageTable.end()) {
		output->verbose(CALL_INFO, 2, 0, "Translation requires new page, creating at physical: %" PRIu64 "\n", nextPageStart);

		resolvedPhysPageStart = nextPageStart;s
		pageTable.insert( std::pair<uint64_t, uint64_t>(virtPageStart, nextPageStart) );
		nextPageStart += pageSize;
	} else {
		resolvedPhysPageStart = findEntry->second;
	}

	output->verbose(CALL_INFO, 2, 0, "Translated physical page to %" PRIu64 " + offset %" PRIu64 " = final physical %" PRIu64 "\n",
		resolvedPhysPageStart, pageOffset, (resolvedPhysPageStart + pageOffset));

	// Reapply the offset to the physical page we just located and we are finished
	return resolvedPhysPageStart + pageOffset;
}
