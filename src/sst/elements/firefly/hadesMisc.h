// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_HADESMISC_H
#define COMPONENTS_FIREFLY_HADESMISC_H

#include <sst/core/elementinfo.h>
#include "sst/elements/hermes/miscapi.h"
#include "hades.h"

using namespace Hermes;

namespace SST {
namespace Firefly {

class HadesMisc : public Misc::Interface
{
  public:
  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        HadesMisc,
        "firefly",
        "hadesMisc",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

    HadesMisc(Component* owner, Params&) : Interface(owner), m_os(NULL) {}
    ~HadesMisc() {}

    virtual void setup() {}
    virtual void finish() {}
    virtual std::string getName() { return "HadesMisc"; }

    virtual void setOS( OS* os ) {
        m_os = static_cast<Hades*>(os);
    }
    void getNumNodes( int* ptr, Callback callback) { 
        *ptr = m_os->getNumNodes();
        callback(0);
    }

    void getNodeNum( int* ptr, Callback callback) { 
        *ptr = m_os->getNodeNum();
        callback(0);
    }
  private:
    Hades*      m_os;
};

}
}

#endif
