from typing import Literal
from pydantic import BaseModel

class BiomeConf(BaseModel):
    id: str                  # e.g., "plains"
    band: Literal["equator", "temperate", "polar"]
    width: float             # 0-1 normalized
    base: int                # Base height
    amp: int                 # Amplitude for hills
    freq: float              # Noise frequency
    color: list[int] = [255, 255, 255]  # RGB color for this biome (default white)

class Preset(BaseModel):
    world_size: int          # e.g., 2048
    sea_level: int = 62
    macro_seed: int
    height_seed: int
    biomes: list[BiomeConf]
    calls: list[dict] = []   # Optional: [{"fn": "carve_cave", "args": {...}}] 