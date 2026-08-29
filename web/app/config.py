import os

CONFIG_DIR = os.environ.get("HEIMDALL_CONFIG_DIR", "/app/config")
CAMERAS_DIR = os.path.join(CONFIG_DIR, "cameras")
HEIMDALL_CONFIG = os.path.join(CONFIG_DIR, "heimdall.jsonc")
LOGS_DIR = os.environ.get("HEIMDALL_LOGS_DIR", "/app/logs")
HEIMDALL_CONTAINER_NAME = os.environ.get("HEIMDALL_CONTAINER", "heimdall")
