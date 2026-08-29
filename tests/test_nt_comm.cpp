#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <optional>
#include <thread>
#include "comm/nt_comm.h"
#include "threat/threat.h"
#include <networktables/NetworkTableInstance.h>

using Catch::Matchers::WithinAbs;

TEST_CASE("NtComm publishes flee vector and reads pose", "[nt]") {
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
    frame.has_threat = true;
    frame.flee_x = -1.f;
    frame.flee_y = 0.f;
    frame.nearest_range = 2.f;
    frame.healthy = true;
    frame.threats.push_back({});
    comm.send_threat_frame(frame);

    auto has = rio->GetBooleanTopic("hasThreat").Subscribe(false);
    auto fx  = rio->GetDoubleTopic("fleeX").Subscribe(0.0);
    bool seen = false;
    for (int i = 0; i < 40; ++i) {
        server.Flush();
        if (has.Get()) { seen = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    REQUIRE(seen);
    REQUIRE_THAT(fx.Get(), WithinAbs(-1.0, 1e-3));

    server.StopServer();
}
