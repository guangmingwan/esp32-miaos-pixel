import importlib.util
from pathlib import Path
import sys
from zipfile import ZipFile


SCRIPT = Path(__file__).parents[1] / "tools" / "build_sd.py"
sys.path.insert(0, str(SCRIPT.parent))
SPEC = importlib.util.spec_from_file_location("build_sd", SCRIPT)
build_sd = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = build_sd
SPEC.loader.exec_module(build_sd)


def test_distribution_registry_is_valid():
    names = [app.name for app in build_sd.APPS]
    assert len(names) == len(set(names))
    assert set(app.category for app in build_sd.APPS) <= set(build_sd.CATEGORIES)
    assert not {"mia_test", "psram_test"} & set(names)
    assert {"usb disk", "usb_wifi"} <= set(names)
    for app in build_sd.APPS:
        if app.source is None:
            assert app.project_dir.is_dir()


def test_create_archive_uses_miaos_layout_and_fresh_manifest(tmp_path, monkeypatch):
    source = tmp_path / "sample.bin"
    source.write_bytes(b"firmware")
    app = build_sd.App("sample", "Utils")
    monkeypatch.setattr(build_sd, "APPS", (app,))
    monkeypatch.setattr(build_sd.App, "artifact", property(lambda self: source))

    output = tmp_path / "sd.zip"
    packed, skipped = build_sd.create_archive(output, 1234567890)

    with ZipFile(output) as archive:
        path = "MiaOS/Utils/sample.app/sample.bin"
        assert path in archive.namelist()
        data = archive.read(path)
    manifest = build_sd.parse_trailer(data)
    assert manifest["category"] == "Utils"
    assert manifest["name"] == "sample"
    assert manifest["build_epoch"] == 1234567890
    assert data[:manifest["image_size"]] == b"firmware"
    assert (packed, skipped) == (1, 0)


def test_create_archive_skips_missing_artifact(tmp_path, monkeypatch):
    monkeypatch.setattr(build_sd, "APPS", (build_sd.App("missing", "Games"),))
    output = tmp_path / "sd.zip"

    packed, skipped = build_sd.create_archive(output, 1234567890)

    with ZipFile(output) as archive:
        assert "MiaOS/Games/" in archive.namelist()
        assert not any(name.endswith(".bin") for name in archive.namelist())
    assert (packed, skipped) == (0, 1)
