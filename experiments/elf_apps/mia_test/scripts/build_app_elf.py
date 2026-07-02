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
    build_dir = Path(env.subst("$BUILD_DIR"))
    toolchain_dir = Path(env.PioPlatform().get_package_dir("toolchain-xtensa-esp32s3"))
    compiler = toolchain_dir / "bin" / "xtensa-esp32s3-elf-gcc"
    output = build_dir / "app.elf"
    app_source = project_dir / "src" / "main.c"

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
        str(app_source),
        "-o",
        str(output),
    ]
    subprocess.run(command, check=True)
    print(f"Built ELF app: {output}")


build_dir = Path(env.subst("$BUILD_DIR"))
source_file = Path(env.subst("$PROJECT_DIR")) / "src" / "main.c"
elf_target = env.Command(str(build_dir / "app.elf"), str(source_file), build_app_elf)
env.AlwaysBuild(elf_target)
env.Default(elf_target)
