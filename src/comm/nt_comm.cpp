#include "nt_comm.h"
#include <chrono>
#include <cstdio>
#include <utility>
#include <vector>

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

    robots_x_ = table->GetDoubleArrayTopic("robots/x").Publish();
    robots_y_ = table->GetDoubleArrayTopic("robots/y").Publish();
    healthy_  = table->GetBooleanTopic("healthy").Publish();

    healthy_.Set(false);
    robots_x_.Set(std::vector<double>{});
    robots_y_.Set(std::vector<double>{});
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
    std::vector<double> xs, ys;
    xs.reserve(frame.threats.size());
    ys.reserve(frame.threats.size());
    for (const auto& t : frame.threats) {
        xs.push_back(t.field_x);
        ys.push_back(t.field_y);
    }
    robots_x_.Set(xs);
    robots_y_.Set(ys);
    healthy_.Set(frame.healthy);
    inst_.Flush();
}
