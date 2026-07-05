from pathlib import Path
import subprocess
from collections.abc import Callable
from typing import Protocol, cast


class Platform(Protocol):
    def get_package_dir(self, name: str) -> str: ...


class Environment(Protocol):
    def subst(self, value: str) -> str: ...
    def PioPlatform(self) -> Platform: ...
    def Command(self, target: str, source: str, action: Callable[..., None]) -> object: ...
    def AlwaysBuild(self, target: object) -> None: ...
    def Default(self, target: object) -> None: ...


import_env = cast(Callable[[str], None], globals()["Import"])
import_env("env")
env = cast(Environment, globals()["env"])


def build_app_elf(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    repo_dir = project_dir.parents[2]
    build_dir = Path(env.subst("$BUILD_DIR"))
    platform = env.PioPlatform()
    toolchain_dir = None
    for package_name in ("toolchain-xtensa-esp32s3", "toolchain-xtensa-esp-elf"):
        try:
            toolchain_dir = Path(platform.get_package_dir(package_name))
            break
        except KeyError:
            continue
    if toolchain_dir is None:
        raise RuntimeError("No Xtensa ESP32 toolchain package found")

    compiler = toolchain_dir / "bin" / "xtensa-esp32s3-elf-gcc"
    output = build_dir / "app.elf"
    sources = sorted(
        path for path in project_dir.joinpath("src").rglob("*.c")
        if path.name != "stub.c" and "third_party" not in path.parts
    )
    if not sources:
        raise RuntimeError("No C sources found under src/")

    command = [
        str(compiler),
        "-nostartfiles",
        "-nostdlib",
        "-fPIC",
        "-shared",
        "-e",
        "app_main",
        "-fdata-sections",
        "-ffunction-sections",
        "-Wl,--gc-sections",
        "-fvisibility=hidden",
        "-Wl,--strip-all",
        "-Wl,--strip-debug",
        "-Wl,--strip-discarded",
        "-Wl,--allow-shlib-undefined",
        "-Dmain=app_main",
        "-I",
        str(repo_dir / "include"),
        "-I",
        str(project_dir / "src"),
    ]
    command.extend(str(path) for path in sources)
    command.extend(["-o", str(output)])
    subprocess.run(command, check=True)
    print(f"Built ELF app: {output}")


build_dir = Path(env.subst("$BUILD_DIR"))
source_dir = Path(env.subst("$PROJECT_DIR")) / "src"
elf_target = env.Command(str(build_dir / "app.elf"), str(source_dir), build_app_elf)
env.AlwaysBuild(elf_target)
env.Default(elf_target)
