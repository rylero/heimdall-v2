#pragma once
#include "app/threat_output.h"
#include "pose/camera_params.h"
#include <networktables/BooleanTopic.h>
#include <networktables/DoubleArrayTopic.h>
#include <networktables/DoubleTopic.h>
#include <networktables/NetworkTableInstance.h>
#include <cstdint>
#include <optional>
#include <string>

// NetworkTables bridge. The roboRIO is the NT server; this process is a client.
//
// Robot publishes:
//   heimdall/pose/x, heimdall/pose/y, heimdall/pose/heading
// Jetson publishes:
//   heimdall/robots/x, heimdall/robots/y  (parallel double arrays, field m)
//   heimdall/healthy
class NtComm : public ThreatOutput {
public:
    struct Config {
        int         team     = 6238;       // SetServerTeam; ignored if server is set
        std::string server;                // e.g. "10.62.38.2" — empty = use team address
        int         port     = 5810;       // NT4
        std::string identity = "heimdall";
        std::string table    = "heimdall";
    };

    explicit NtComm(Config config);

    std::optional<TimestampedPose> try_recv_pose();
    void send_threat_frame(const ThreatFrame& frame) override;

private:
    Config                     config_;
    nt::NetworkTableInstance   inst_;
    nt::DoubleSubscriber       pose_x_;
    nt::DoubleSubscriber       pose_y_;
    nt::DoubleSubscriber       pose_heading_;
    nt::DoubleArrayPublisher   robots_x_;
    nt::DoubleArrayPublisher   robots_y_;
    nt::BooleanPublisher       healthy_;
    int64_t                    last_pose_change_us_ = 0;
};
