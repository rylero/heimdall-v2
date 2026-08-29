#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cmath>
#include <thread>
#include <networktables/NetworkTableInstance.h>

static std::atomic<bool> g_running{true};
static void shutdown(int) { g_running = false; }

int main() {
    std::signal(SIGINT, shutdown);

    auto inst = nt::NetworkTableInstance::GetDefault();
    inst.StartServer();
    auto table = inst.GetTable("heimdall");
    auto pose_x = table->GetDoubleTopic("pose/x").Publish();
    auto pose_y = table->GetDoubleTopic("pose/y").Publish();
    auto pose_h = table->GetDoubleTopic("pose/heading").Publish();
    auto has    = table->GetBooleanTopic("hasThreat").Subscribe(false);
    auto flee_x = table->GetDoubleTopic("fleeX").Subscribe(0.0);
    auto flee_y = table->GetDoubleTopic("fleeY").Subscribe(0.0);
    auto range  = table->GetDoubleTopic("nearestRange").Subscribe(0.0);

    std::printf("Mock robot NT server on :5810. Point heimdall nt.server at 127.0.0.1\n");

    uint64_t frame = 0;
    auto next = std::chrono::steady_clock::now();
    bool last_threat = false;

    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        if (now >= next) {
            const float t = static_cast<float>(frame) * 0.02f;
            pose_x.Set(3.0 + std::cos(t * 0.2));
            pose_y.Set(3.0 + std::sin(t * 0.2));
            pose_h.Set(t * 0.2);
            inst.Flush();
            ++frame;
            next += std::chrono::milliseconds(20);
        }

        const bool threat = has.Get();
        if (threat != last_threat) {
            last_threat = threat;
            std::printf("hasThreat=%d flee=(%.2f, %.2f) range=%.2f\n",
                        (int)threat, flee_x.Get(), flee_y.Get(), range.Get());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    inst.StopServer();
    return 0;
}
