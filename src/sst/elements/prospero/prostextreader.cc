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
#include "prostextreader.h"

using namespace SST::Prospero;


ProsperoTextTraceReader::ProsperoTextTraceReader( ComponentId_t id, Params& params, Output* out ) :
	ProsperoTraceReader(id, params, out) {

	std::string traceFile = params.find<std::string>("file", "");
	traceInput = fopen(traceFile.c_str(), "rt");

	if(NULL == traceInput) {
            output->fatal(CALL_INFO, -1, "%s, Fatal: Unable to open file: %s in text reader.\n",
                    getName().c_str(), traceFile.c_str());
	}

}

ProsperoTextTraceReader::~ProsperoTextTraceReader() {
	if(NULL != traceInput) {
		fclose(traceInput);
	}
}

ProsperoTraceEntry* ProsperoTextTraceReader::readNextEntry() {
	uint64_t reqAddress = 0;
	uint64_t reqCycles  = 0;
	char _reqType[2];
	std::string reqType;
	uint32_t reqLength  = 0;
	uint64_t instPtr  = 0;
	ProsperoTraceEntryOperation op;

	// 여기서 pc도 읽어야 함
	if(EOF == fscanf(traceInput, "%" PRIu64 " %s %" PRIu64 " %" PRIu32 " %" PRIu64 "",
		&reqCycles, _reqType, &reqAddress, &reqLength, &instPtr) ) {
		return NULL;
	} else {
		reqType = _reqType;
		if( reqType == "R"){
			op = READ;
		}
		else if( reqType == "W"){
			op = WRITE;
		}
		else if( reqType == "WR"){
			op = WREAD;
			output->verbose(CALL_INFO, 4, 0, "Try to read weight data\n");
		}
		else if( reqType == "WW"){
			op = WWRITE;
			output->verbose(CALL_INFO, 4, 0, "Try to write weight data\n");
		}

		return new ProsperoTraceEntry(reqCycles, reqAddress,
			reqLength, op, instPtr);
	}
}
