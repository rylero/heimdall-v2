#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "app/pose_buffer.h"

using Catch::Matchers::WithinAbs;

static TimestampedPose make_tp(uint64_t recv_ns, float x, float y, float heading = 0.f) {
    return TimestampedPose{RobotPose{x, y, heading, 0}, recv_ns};
}

TEST_CASE("PoseBuffer: empty returns zero pose", "[pose_buffer]") {
    PoseBuffer buf;
    RobotPose p = buf.closest(1'000'000ULL);
    REQUIRE_THAT(p.x,       WithinAbs(0.f, 1e-6f));
    REQUIRE_THAT(p.y,       WithinAbs(0.f, 1e-6f));
    REQUIRE_THAT(p.heading, WithinAbs(0.f, 1e-6f));
}

TEST_CASE("PoseBuffer: single entry always returned", "[pose_buffer]") {
    PoseBuffer buf;
    buf.push(make_tp(1'000'000ULL, 3.f, 4.f, 1.f));

    RobotPose p1 = buf.closest(0ULL);
    REQUIRE_THAT(p1.x, WithinAbs(3.f, 1e-5f));
    REQUIRE_THAT(p1.y, WithinAbs(4.f, 1e-5f));

    RobotPose p2 = buf.closest(1'000'000ULL);
    REQUIRE_THAT(p2.x, WithinAbs(3.f, 1e-5f));

    RobotPose p3 = buf.closest(999'999'999'999ULL);
    REQUIRE_THAT(p3.x, WithinAbs(3.f, 1e-5f));
}

TEST_CASE("PoseBuffer: two entries picks nearest", "[pose_buffer]") {
    PoseBuffer buf;
    buf.push(make_tp(10'000'000ULL, 1.f, 0.f));
    buf.push(make_tp(30'000'000ULL, 2.f, 0.f));

    RobotPose p1 = buf.closest(15'000'000ULL);
    REQUIRE_THAT(p1.x, WithinAbs(1.f, 1e-5f));

    RobotPose p2 = buf.closest(25'000'000ULL);
    REQUIRE_THAT(p2.x, WithinAbs(2.f, 1e-5f));
}

TEST_CASE("PoseBuffer: ring wrap evicts oldest entry", "[pose_buffer]") {
    PoseBuffer buf;
    constexpr size_t N = PoseBuffer::N;

    for (size_t i = 0; i < N; ++i)
        buf.push(make_tp(static_cast<uint64_t>(i) * 1'000'000ULL, static_cast<float>(i), 0.f));

    buf.push(make_tp(static_cast<uint64_t>(N) * 1'000'000ULL, static_cast<float>(N), 0.f));

    RobotPose p = buf.closest(0ULL);
    REQUIRE_THAT(p.x, WithinAbs(1.f, 1e-5f));

    RobotPose newest = buf.closest(static_cast<uint64_t>(N) * 1'000'000ULL);
    REQUIRE_THAT(newest.x, WithinAbs(static_cast<float>(N), 1e-5f));
}

TEST_CASE("PoseBuffer: selects pose matching camera capture time at 50Hz", "[pose_buffer]") {
    PoseBuffer buf;
    for (int i = 0; i < 10; ++i)
        buf.push(make_tp(static_cast<uint64_t>(i) * 20'000'000ULL, static_cast<float>(i), 0.f));

    const uint64_t capture_ns = 100'000'000ULL;
    RobotPose p = buf.closest(capture_ns);
    REQUIRE_THAT(p.x, WithinAbs(5.f, 1e-5f));
}
