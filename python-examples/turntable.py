import logging
import shutil
import tempfile
from math import pi
from pathlib import Path
from subprocess import PIPE, Popen
from typing import Any, Callable, Iterable, Iterator, Optional, Sequence
from urllib import request

import f3d
import numpy as np
from tqdm import tqdm
from transforms3d._gohlketransforms import rotation_matrix

FFMPEG_ARGS = [
    *("-profile:v", "main", "-c:v", "libx264", "-pix_fmt", "yuv420p"),
    *("-crf", "1"),
    *("-loglevel", "error"),
]


def main():
    resolution = 1280, 720
    fps = 30
    duration = 5
    model_fn = "https://github.com/KhronosGroup/glTF-Sample-Models/raw/master/2.0/DamagedHelmet/glTF-Binary/DamagedHelmet.glb"
    hdri_fn = "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/2k/industrial_sunset_02_puresky_2k.hdr"

    video_out_fn = out_video_filename("f3d-turntable", resolution, fps, duration)
    offscreen = False

    options = {
        "render.hdri.file": download_file_if_url(hdri_fn),
        "render.hdri.ambient": True,
        "render.background.skybox": True,
        "render.background.blur": True,
        "render.effect.tone-mapping": True,
        "render.effect.ambient-occlusion": True,
        "render.effect.translucency-support": True,
        "render.effect.anti-aliasing": True,
        "interactor.axis": True,
        "scene.up-direction": "+y",
    }

    frames = render_turntable(
        download_file_if_url(model_fn),
        options,
        geometry_only=False,
        resolution=resolution,
        duration=duration,
        fps=fps,
        offscreen=offscreen,
        start_position=(1, 1, 1),
    )

    frames = tqdm(frames, total=int(duration * fps))

    ffmpeg_encode_sequence(
        (im.content for im in frames),
        resolution,
        fps,
        video_out_fn,
        FFMPEG_ARGS,
        vflip=True,
    )
    logging.info(f"saved video to `{video_out_fn}`")


def out_video_filename(
    basename: str,
    resolution: tuple[int, int],
    fps: float,
    duration: float,
    into: Path = Path(tempfile.gettempdir()),
    ext: str = ".mp4",
):
    w, h = resolution
    name = f"{basename}-{w}x{h}@{fps:g}fps-{duration:g}s{ext}"
    return into / name


Opts = dict[str, Any]


def render_turntable(
    model_fn: str,
    options: Opts,
    *,
    geometry_only: bool = False,
    resolution: tuple[int, int] = (960, 540),
    fps: int = 20,
    duration: float = 5,
    start_position: tuple[float, float, float] = (1, 1, 1),
    zoom_factor: Optional[float] = 0.9,
    turns: float = 1,
    offscreen: bool = True,
) -> Iterator[f3d.Image]:
    engine = f3d.Engine(f3d.window.NATIVE_OFFSCREEN if offscreen else f3d.window.NATIVE)
    engine.options.update(options)
    engine.window.size = resolution
    if geometry_only:
        engine.loader.load_geometry(model_fn)
    else:
        engine.loader.load_scene(model_fn)

    engine.window.camera.position = start_position
    if zoom_factor:
        engine.window.camera.reset_to_bounds(zoom_factor=zoom_factor)
    cam_state_f = turntable_cam_state_func(
        engine.window.camera.state,
        up_direction_axis(options.get("scene.up-direction", "y")),
        turns=turns,
    )

    frame_count = int(round(duration * fps))
    for i in range(frame_count):
        t = i / frame_count
        yield f3d_render_frame(engine, cam_state_f, options, t)


def turntable_cam_state_func(
    initial_state: f3d.CameraState,
    axis: tuple[float, float, float],
    turns: float = 1,
) -> Callable[[float], f3d.CameraState]:
    initial_foc = np.array(initial_state.foc)
    initial_pos = np.array(initial_state.pos)
    initial_up = np.array(initial_state.up)

    def transform_point(point, matrix):
        new_point = np.array([*point[:3], 1]) @ matrix.T
        return new_point[:3] / new_point[3]

    def interpolate_state(t: float):
        m = rotation_matrix(2 * pi * t * turns, axis, initial_foc)
        pos = transform_point(initial_pos, m)
        foc = transform_point(initial_foc, m)
        up = transform_point(initial_pos + initial_up, m) - pos
        return f3d.CameraState(pos, foc, up, initial_state.angle)

    return interpolate_state


def up_direction_axis(up_direction: str):
    up_axes = {
        "x": (+1, 0, 0),
        "y": (0, +1, 0),
        "z": (0, 0, +1),
    }
    return up_axes[up_direction.lstrip("+-").lower()]


def f3d_render_frame(
    engine: f3d.Engine,
    cam: Optional[Callable[[float], f3d.CameraState] | f3d.CameraState] = None,
    opts: Optional[Callable[[float], Opts] | Opts] = None,
    t: float = 0,
) -> f3d.Image:
    if opts is not None:
        engine.options.update(opts(t) if callable(opts) else opts)
    if cam is not None:
        engine.window.camera.state = cam(t) if callable(cam) else cam
    return engine.window.render_to_image()


def ffmpeg_encode_sequence(
    rgb_frames: Iterable[bytes],
    resolution: tuple[int, int],
    fps: float,
    out_path: str | Path,
    output_args: Sequence[str],
    vflip: bool = False,
    ffmpeg_executable: str = "ffmpeg",
):
    def build_command() -> Iterator[str]:
        res = f"{resolution[0]}x{resolution[1]}"
        yield ffmpeg_executable
        yield from ("-f", "rawvideo", "-pix_fmt", "rgb24", "-s", res)
        yield from ("-r", f"{fps}", "-i", "-")
        if vflip:
            yield from ("-vf", "vflip")
        if output_args:
            yield from output_args
        yield from (str(out_path), "-y")

    ffmpeg_cmd = list(build_command())
    proc = Popen(ffmpeg_cmd, stdin=PIPE)
    for frame_data in rgb_frames:
        proc.stdin.write(frame_data)
        proc.stdin.flush()
    proc.stdin.close()
    proc.wait()


def download_file_if_url(url_or_path: str):
    if Path(url_or_path).exists():
        return url_or_path
    else:
        return download_file(url_or_path)


def download_file(url: str):
    fn = Path(tempfile.gettempdir()) / Path(url).name
    if not fn.is_file():
        logging.info(f"downloading `{url}` to `{fn}`")
        req = request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with request.urlopen(req) as response, open(fn, "wb") as f:
            shutil.copyfileobj(response, f)
    else:
        logging.info(f"using `{fn}` for `{url}`")

    if not fn.is_file():
        raise IOError(f"could not retrieve {url}")

    return str(fn)


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    main()
