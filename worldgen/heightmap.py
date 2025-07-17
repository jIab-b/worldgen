import numpy as np
from .config_schema import Preset
from .biome import BiomeMask
from .noise import fbm_2D, fbm_2D_cpu
import cupy as cp  # type: ignore


class HeightField:
    """
    Generates a heightmap for the world based on biome masks and fBm noise.
    """

    def __init__(self, preset: Preset, mask: np.ndarray):
        self.preset = preset
        self.mask = mask
        self.size = preset.world_size

    def build(self) -> np.ndarray:
        """
        Create a (size x size) heightmap array.
        """
        # Prepare coordinate grids
        xs = np.arange(self.size)
        zs = np.arange(self.size)
        xs, zs = np.meshgrid(xs, zs, indexing="ij")

        height = np.zeros((self.size, self.size), dtype=np.int32)

        unique_biomes = np.unique(self.mask)
        for biome_id in unique_biomes:
            idx = (self.mask == biome_id)
            if not np.any(idx):
                continue
            biome = self.preset.biomes[biome_id]  # Assume id matches index
            xs_slice, zs_slice = xs[idx], zs[idx]

            # GPU compute
            try:
                xs_cp, zs_cp = cp.array(xs_slice), cp.array(zs_slice)
                noise = fbm_2D(self.preset.height_seed ^ biome_id, xs_cp, zs_cp, biome.freq, octaves=5)
                noise_min, noise_max = noise.min(), noise.max()
                if noise_max != noise_min:
                    noise = (noise - noise_min) / (noise_max - noise_min)  # Normalize [0,1]
                else:
                    noise = cp.zeros_like(xs_cp)
            except Exception: # Fallback to CPU
                noise = fbm_2D_cpu(self.preset.height_seed ^ biome_id, xs_slice, zs_slice, biome.freq, octaves=5)
                noise_min, noise_max = noise.min(), noise.max()
                if noise_max != noise_min:
                    noise = (noise - noise_min) / (noise_max - noise_min)
                else:
                    noise = np.zeros_like(xs_slice)

            height[idx] = biome.base + (biome.amp * noise).astype(int)
        return height 