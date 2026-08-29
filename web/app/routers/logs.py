from fastapi import APIRouter
from fastapi.responses import StreamingResponse
import docker
from docker.errors import NotFound
from app import config

router = APIRouter()


def _container_logs():
    try:
        client = docker.from_env()
        container = client.containers.get(config.HEIMDALL_CONTAINER_NAME)
        for chunk in container.logs(stream=True, follow=True, tail=200):
            line = chunk.decode("utf-8", errors="replace").rstrip()
            if line:
                yield f"data: {line}\n\n"
    except NotFound:
        yield f"data: [container '{config.HEIMDALL_CONTAINER_NAME}' not found]\n\n"
    except Exception as e:
        yield f"data: [docker error: {e}]\n\n"


@router.get("/container")
def stream_container():
    return StreamingResponse(
        _container_logs(),
        media_type="text/event-stream",
        headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
    )
