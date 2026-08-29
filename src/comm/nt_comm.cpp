#include "nt_comm.h"
#include <chrono>
#include <cstdio>
#include <utility>

NtComm::NtComm(Config config)
    : config_(std::move(config)),
      inst_(nt::NetworkTableInstance::GetDefault())
{
    inst_.StartClient4(config_.identity);
    if (!config_.server.empty()) {
        inst_.SetServer(config_.server, config_.port);
        std::printf("[nt] client '%s' → %s:%d table=/%s\n",
                    config_.identity.c_str(), config_.server.c_str(),
                    config_.port, config_.table.c_str());
    } else {
        inst_.SetServerTeam(config_.team, config_.port);
        inst_.StartDSClient(config_.port);
        std::printf("[nt] client '%s' → team %d port %d table=/%s\n",
                    config_.identity.c_str(), config_.team,
                    config_.port, config_.table.c_str());
    }

    auto table = inst_.GetTable(config_.table);

    pose_x_       = table->GetDoubleTopic("pose/x").Subscribe(0.0);
    pose_y_       = table->GetDoubleTopic("pose/y").Subscribe(0.0);
    pose_heading_ = table->GetDoubleTopic("pose/heading").Subscribe(0.0);

    has_threat_     = table->GetBooleanTopic("hasThreat").Publish();
    flee_x_         = table->GetDoubleTopic("fleeX").Publish();
    flee_y_         = table->GetDoubleTopic("fleeY").Publish();
    nearest_range_  = table->GetDoubleTopic("nearestRange").Publish();
    healthy_        = table->GetBooleanTopic("healthy").Publish();
    threat_count_   = table->GetIntegerTopic("threatCount").Publish();

    healthy_.Set(false);
    has_threat_.Set(false);
    inst_.Flush();
}

std::optional<TimestampedPose> NtComm::try_recv_pose() {
    const auto x = pose_x_.GetAtomic();
    if (x.time == 0) return std::nullopt;
    if (static_cast<int64_t>(x.time) == last_pose_change_us_) return std::nullopt;
    last_pose_change_us_ = static_cast<int64_t>(x.time);

    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const uint64_t recv_ns =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());

    TimestampedPose p;
    p.pose.x       = static_cast<float>(x.value);
    p.pose.y       = static_cast<float>(pose_y_.Get());
    p.pose.heading = static_cast<float>(pose_heading_.Get());
    p.jetson_recv_ns = recv_ns;
    return p;
}

void NtComm::send_threat_frame(const ThreatFrame& frame) {
    has_threat_.Set(frame.has_threat);
    flee_x_.Set(frame.flee_x);
    flee_y_.Set(frame.flee_y);
    nearest_range_.Set(frame.nearest_range);
    healthy_.Set(frame.healthy);
    threat_count_.Set(static_cast<int64_t>(frame.threats.size()));
    inst_.Flush();
}
