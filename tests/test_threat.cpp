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

TEST_CASE("empty detections produce no threat", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({}, pose, {});
    REQUIRE_FALSE(frame.has_threat);
    REQUIRE(frame.threats.empty());
    REQUIRE_THAT(frame.flee_x, WithinAbs(0.f, 1e-6f));
    REQUIRE_THAT(frame.flee_y, WithinAbs(0.f, 1e-6f));
}

TEST_CASE("robot in front of us flees backward", "[threat]") {
    // Us at origin heading +X. Other robot 2 m ahead on field +X.
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 2.f, 0.f)}, pose, {});
    REQUIRE(frame.has_threat);
    REQUIRE(frame.threats.size() == 1);
    REQUIRE_THAT(frame.nearest_range, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(0.f, 1e-4f));
    REQUIRE_THAT(frame.flee_x, WithinAbs(-1.f, 1e-4f));
    REQUIRE_THAT(frame.flee_y, WithinAbs(0.f,  1e-4f));
}

TEST_CASE("robot to the left flees right", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 0.f, 3.f)}, pose, {});
    REQUIRE(frame.has_threat);
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(0.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(3.f, 1e-4f));
    REQUIRE_THAT(frame.flee_x, WithinAbs(0.f,  1e-4f));
    REQUIRE_THAT(frame.flee_y, WithinAbs(-1.f, 1e-4f));
}

TEST_CASE("heading rotates field detections into robot frame", "[threat]") {
    // Facing +Y (pi/2). A field point 2 m along +Y is in front of us.
    RobotPose pose{0.f, 0.f, static_cast<float>(M_PI / 2)};
    auto frame = select_threats({fd(0, 0.f, 2.f)}, pose, {});
    REQUIRE(frame.has_threat);
    REQUIRE_THAT(frame.threats[0].robot_x, WithinAbs(2.f, 1e-4f));
    REQUIRE_THAT(frame.threats[0].robot_y, WithinAbs(0.f, 1e-4f));
    REQUIRE_THAT(frame.flee_x, WithinAbs(-1.f, 1e-4f));
}

TEST_CASE("nearest threat wins the flee vector", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    auto frame = select_threats({fd(0, 4.f, 0.f), fd(0, 0.f, 1.5f)}, pose, {});
    REQUIRE(frame.has_threat);
    REQUIRE(frame.threats.size() == 2);
    REQUIRE_THAT(frame.nearest_range, WithinAbs(1.5f, 1e-4f));
    REQUIRE_THAT(frame.flee_y, WithinAbs(-1.f, 1e-4f));
    REQUIRE_THAT(frame.flee_x, WithinAbs(0.f,  1e-4f));
}

TEST_CASE("low confidence detections are dropped", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.min_confidence = 0.5f;
    auto frame = select_threats({fd(0, 2.f, 0.f, 0.2f)}, pose, cfg);
    REQUIRE_FALSE(frame.has_threat);
}

TEST_CASE("class_ids allowlist drops other classes", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.class_ids = {1};
    auto frame = select_threats({fd(0, 2.f, 0.f), fd(1, 0.f, 2.f)}, pose, cfg);
    REQUIRE(frame.has_threat);
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
    REQUIRE_FALSE(frame.has_threat);
}

TEST_CASE("detections beyond max_range are ignored", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.max_range = 5.f;
    auto frame = select_threats({fd(0, 9.f, 0.f)}, pose, cfg);
    REQUIRE_FALSE(frame.has_threat);
}

TEST_CASE("merge_radius collapses multi-camera duplicates", "[threat]") {
    RobotPose pose{0.f, 0.f, 0.f};
    ThreatConfig cfg;
    cfg.merge_radius = 0.4f;
    auto frame = select_threats({
        fd(0, 2.00f, 0.00f, 0.7f),
        fd(0, 2.10f, 0.05f, 0.95f),  // same object, other camera, higher conf
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

TEST_CASE("flee vector is unit length", "[threat]") {
    RobotPose pose{1.f, 2.f, 0.4f};
    auto frame = select_threats({fd(0, 3.f, 5.f)}, pose, {});
    REQUIRE(frame.has_threat);
    const float mag = std::hypot(frame.flee_x, frame.flee_y);
    REQUIRE_THAT(mag, WithinAbs(1.f, 1e-4f));
}
