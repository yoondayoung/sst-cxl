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

#ifndef _H_VANADIS_SYSCALL_SET_THREAD_AREA
#define _H_VANADIS_SYSCALL_SET_THREAD_AREA

#include "os/voscallev.h"

namespace SST {
namespace Vanadis {

class VanadisSyscallSetThreadAreaEvent : public VanadisSyscallEvent {
public:
    VanadisSyscallSetThreadAreaEvent() : VanadisSyscallEvent() {}
    VanadisSyscallSetThreadAreaEvent(uint32_t core, uint32_t thr, VanadisOSBitType bittype, uint64_t ta)
        : VanadisSyscallEvent(core, thr, bittype), ta_ptr(ta) {}

    VanadisSyscallOp getOperation() override { return SYSCALL_OP_SET_THREAD_AREA; }

    uint64_t getThreadAreaInfoPtr() const { return ta_ptr; }

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        VanadisSyscallEvent::serialize_order(ser);
        ser& ta_ptr;
    }
    ImplementSerializable(SST::Vanadis::VanadisSyscallSetThreadAreaEvent);
    uint64_t ta_ptr;
};

} // namespace Vanadis
} // namespace SST

#endif
