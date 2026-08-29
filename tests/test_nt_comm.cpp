#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <optional>
#include <thread>
#include <vector>
#include "comm/nt_comm.h"
#include "threat/threat.h"
#include <networktables/NetworkTableInstance.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("NtComm publishes robot position arrays and reads pose", "[nt]") {
    auto server = nt::NetworkTableInstance::Create();
    server.StartServer("heimdall-nt-test.json", "127.0.0.1", 1735, 5812);

    auto rio = server.GetTable("heimdall");
    auto pose_x = rio->GetDoubleTopic("pose/x").Publish();
    auto pose_y = rio->GetDoubleTopic("pose/y").Publish();
    auto pose_h = rio->GetDoubleTopic("pose/heading").Publish();

    NtComm::Config cfg;
    cfg.server = "127.0.0.1";
    cfg.port   = 5812;
    cfg.identity = "heimdall-test";
    NtComm comm(cfg);

    pose_x.Set(1.5);
    pose_y.Set(-2.0);
    pose_h.Set(0.3);
    server.Flush();

    std::optional<TimestampedPose> got;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline && !got) {
        pose_x.Set(1.5);
        pose_y.Set(-2.0);
        pose_h.Set(0.3);
        server.Flush();
        got = comm.try_recv_pose();
        if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    REQUIRE(got.has_value());
    REQUIRE_THAT(got->pose.x, WithinAbs(1.5f, 1e-3f));
    REQUIRE_THAT(got->pose.y, WithinAbs(-2.0f, 1e-3f));
    REQUIRE_THAT(got->pose.heading, WithinAbs(0.3f, 1e-3f));

    ThreatFrame frame;
    frame.healthy = true;
    frame.threats.push_back(Threat{.field_x = 4.0f, .field_y = 1.5f});
    frame.threats.push_back(Threat{.field_x = 2.0f, .field_y = -0.5f});
    comm.send_threat_frame(frame);

    auto xs = rio->GetDoubleArrayTopic("robots/x").Subscribe(std::vector<double>{});
    auto ys = rio->GetDoubleArrayTopic("robots/y").Subscribe(std::vector<double>{});
    bool seen = false;
    std::vector<double> got_x;
    for (int i = 0; i < 40; ++i) {
        server.Flush();
        got_x = xs.Get();
        if (got_x.size() == 2) { seen = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    REQUIRE(seen);
    auto got_y = ys.Get();
    REQUIRE(got_y.size() == 2);
    REQUIRE_THAT(got_x[0], WithinAbs(4.0, 1e-3));
    REQUIRE_THAT(got_y[0], WithinAbs(1.5, 1e-3));
    REQUIRE_THAT(got_x[1], WithinAbs(2.0, 1e-3));
    REQUIRE_THAT(got_y[1], WithinAbs(-0.5, 1e-3));

    server.StopServer();
}
