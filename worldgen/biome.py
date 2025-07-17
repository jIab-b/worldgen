import numpy as np
from .config_schema import Preset
import random
from .noise import fbm_2D_cpu


class BiomeMask:
    """
    Builds a 2D mask of biome indices based on Preset.biomes and macro_seed.
    """

    def __init__(self, preset: Preset):
        self.preset = preset
        self.size = preset.world_size

    def build(self) -> np.ndarray:
        """Create (size x size) biome index mask."""
        mask = np.zeros((self.size, self.size), dtype=int)
        total_width = sum(b.width for b in self.preset.biomes)
        cum_widths = np.cumsum([b.width / total_width for b in self.preset.biomes])  # Normalized cumulative

        # Add 1D noise for irregular bands (along z)
        random.seed(self.preset.macro_seed)
        noise_scale = 0.1  # Adjust for edge irregularity
        zs_norm = np.linspace(0, 1, self.size)  # Normalized z
        band_noise = fbm_2D_cpu(self.preset.macro_seed, np.zeros(self.size), zs_norm, freq=0.01, octaves=3) * noise_scale

        for x in range(self.size):
            for z in range(self.size):
                pos = zs_norm[z] + band_noise[z]  # Perturbed position
                for i, cum in enumerate(cum_widths):
                    if pos <= cum:
                        mask[x, z] = i
                        break
        return mask 