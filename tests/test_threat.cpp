#define _USE_MATH_DEFINES
#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "pose/field_detection.h"
#include "threat/threat.h"

using Catch::Matchers::WithinAbs;

static FieldDetection fd(int class_id, float x, float y, float conf = 0.9f) {
    return {class_id, x, y, conf};
}

TEST_CASE("empty detections produce no robots", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({}, pose, {});
    REQUIRE(frame.threats.empty());
}

TEST_CASE("robot in front is +X in the robot frame", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 2.f, 0.f)}, pose, {});
    REQUIRE(frame.threats.size() == 1);
    REQUIRE_THAT(frame.threats[0].field_x, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].field_y, WithinAbs(0.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(0.f, 1e-4f));
}

TEST_CASE("robot to the left is +Y in the robot frame", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 0.f, 3.f)}, pose, {});
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(0.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(3.f, 1e-4f));
}

TEST_CASE("heading rotates field detections into robot frame", "[threat]") {
    RobotPose pose{0.f, 0.f, static_cast<float>(M_PI / 2)};
    auto frame = select_threats({fd(0, 0.f, 2.f)}, pose, {});
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(0.f, 1e-4f));
}

TEST_CASE("two robots stay as two positions", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 4.f, 0.f), fd(0, 0.f, 1.5f)}, pose, {});
    REQUIRE(frame.threats.size() == 2);
}

TEST_CASE("low confidence detections are dropped", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.min_confidence = 0.5f;
    auto frame = select_threats({fd(0, 2.f, 0.f, 0.2f)}, pose, cfg);
    REQUIRE(frame.threats.empty());
}

TEST_CASE("class_ids allowlist drops other classes", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.class_ids = {1};
    auto frame = select_threats({fd(0, 2.f, 0.f), fd(1, 0.f, 2.f)}, pose, cfg);
    REQUIRE(frame.threats.size() == 1);
    REQUIRE(frame.threats[0].class_id == 1);
}

TEST_CASE("empty class_ids keeps every class", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 2.f, 0.f), fd(7, 0.f, 2.f)}, pose, {});
    REQUIRE(frame.threats.size() == 2);
}

TEST_CASE("detections inside min_range are ignored", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.min_range = 0.5f;
    auto frame = select_threats({fd(0, 0.1f, 0.f)}, pose, cfg);
    REQUIRE(frame.threats.empty());
}

TEST_CASE("detections beyond max_range are ignored", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.max_range = 5.f;
    auto frame = select_threats({fd(0, 9.f, 0.f)}, pose, cfg);
    REQUIRE(frame.threats.empty());
}

TEST_CASE("merge_radius collapses multi-camera duplicates", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.merge_radius = 0.4f;
    auto frame = select_threats({
        fd(0, 2.00f, 0.00f, 0.7f),
        fd(0, 2.10f, 0.05f, 0.95f),
    }, pose, cfg);
    REQUIRE(frame.threats.size() == 1);
    REQUIRE_THAT(frame.threats[0].confidence, WithinAbs(0.95f, 1e-6f));
}

TEST_CASE("merge_radius keeps two distinct robots", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.merge_radius = 0.4f;
    auto frame = select_threats({fd(0, 2.f, 0.f), fd(0, 2.f, 2.f)}, pose, cfg);
    REQUIRE(frame.threats.size() == 2);
}
