#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cmath>
#include <thread>
#include <vector>
#include <networktables/BooleanTopic.h>
#include <networktables/DoubleArrayTopic.h>
#include <networktables/DoubleTopic.h>
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
    auto robots_x = table->GetDoubleArrayTopic("robots/x").Subscribe(std::vector<double>{});
    auto robots_y = table->GetDoubleArrayTopic("robots/y").Subscribe(std::vector<double>{});

    std::printf("Mock robot NT server on :5810. Point heimdall nt.server at 127.0.0.1\n");

    uint64_t frame = 0;
    auto next = std::chrono::steady_clock::now();
    size_t last_n = static_cast<size_t>(-1);

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

        const auto xs = robots_x.Get();
        if (xs.size() != last_n) {
            last_n = xs.size();
            const auto ys = robots_y.Get();
            std::printf("robots=%zu", xs.size());
            for (size_t i = 0; i < xs.size() && i < ys.size(); ++i)
                std::printf("  (%.2f, %.2f)", xs[i], ys[i]);
            std::printf("\n");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    inst.StopServer();
    return 0;
}
