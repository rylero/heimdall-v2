#pragma once
#include "threat/threat.h"

struct ThreatOutput {
    virtual void send_threat_frame(const ThreatFrame& frame) = 0;
    virtual ~ThreatOutput() = default;
};
