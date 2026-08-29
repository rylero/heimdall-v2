#include "app/heimdall_app.h"
#include "config/app_config_loader.h"
#include <csignal>
#include <cstdio>

static HeimdallApp* g_app = nullptr;

static void shutdown(int) {
    if (g_app) g_app->stop();
}

int main() {
    AppConfig cfg = load_app_config("config/heimdall.jsonc");

    HeimdallApp app(std::move(cfg));
    g_app = &app;
    std::signal(SIGINT, shutdown);
    std::printf("heimdall-v2 starting. Ctrl+C to stop.\n");
    app.run();
    return 0;
}
