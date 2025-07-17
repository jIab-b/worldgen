import json
from pathlib import Path
from .config_schema import Preset


def load_preset(path: str) -> Preset:
    data = json.loads(Path(path).read_text())
    return Preset.parse_obj(data)


def save_preset(preset: Preset, path: str):
    Path(path).write_text(preset.json(indent=2)) 