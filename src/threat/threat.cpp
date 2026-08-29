#include "threat.h"
#include <algorithm>
#include <cmath>
#include <vector>

static bool class_allowed(int class_id, const std::vector<int>& allow) {
    if (allow.empty()) return true;
    return std::find(allow.begin(), allow.end(), class_id) != allow.end();
}

static float dist2(float ax, float ay, float bx, float by) {
    const float dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy;
}

ThreatFrame select_threats(
    const std::vector<FieldDetection>& field_dets,
    const RobotPose&                   pose,
    const ThreatConfig&                cfg
) {
    ThreatFrame out;

    const float ch = std::cos(pose.heading);
    const float sh = std::sin(pose.heading);

    std::vector<Threat> candidates;
    candidates.reserve(field_dets.size());

    for (const auto& fd : field_dets) {
        if (fd.confidence < cfg.min_confidence) continue;
        if (!class_allowed(fd.class_id, cfg.class_ids)) continue;

        const float dx = fd.x - pose.x;
        const float dy = fd.y - pose.y;
        // Field → robot: +X forward, +Y left.
        const float rx =  ch * dx + sh * dy;
        const float ry = -sh * dx + ch * dy;
        const float range = std::hypot(rx, ry);
        if (range < cfg.min_range) continue;
        if (cfg.max_range > 0.f && range > cfg.max_range) continue;

        candidates.push_back(Threat{
            .class_id   = fd.class_id,
            .field_x    = fd.x,
            .field_y    = fd.y,
            .robot_x    = rx,
            .robot_y    = ry,
            .range      = range,
            .confidence = fd.confidence,
        });
    }

    // Greedy field-space NMS: keep the highest-confidence of each cluster.
    if (cfg.merge_radius > 0.f && candidates.size() > 1) {
        std::sort(candidates.begin(), candidates.end(),
                  [](const Threat& a, const Threat& b) {
                      return a.confidence > b.confidence;
                  });
        const float r2 = cfg.merge_radius * cfg.merge_radius;
        std::vector<Threat> merged;
        merged.reserve(candidates.size());
        for (const auto& c : candidates) {
            bool dup = false;
            for (const auto& kept : merged) {
                if (dist2(c.field_x, c.field_y, kept.field_x, kept.field_y) <= r2) {
                    dup = true;
                    break;
                }
            }
            if (!dup) merged.push_back(c);
        }
        candidates = std::move(merged);
    }

    out.threats = std::move(candidates);
    if (out.threats.empty()) return out;

    const auto nearest = std::min_element(
        out.threats.begin(), out.threats.end(),
        [](const Threat& a, const Threat& b) { return a.range < b.range; });

    out.has_threat    = true;
    out.nearest_range = nearest->range;
    if (nearest->range > 0.f) {
        out.flee_x = -nearest->robot_x / nearest->range;
        out.flee_y = -nearest->robot_y / nearest->range;
    }
    return out;
}
