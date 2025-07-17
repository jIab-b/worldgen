import numpy as np
from pathlib import Path
from .config_schema import Preset


class Voxelizer:
    """
    Fills 3D voxel chunks based on heightmap and biome data.
    """

    def __init__(self, preset: Preset, height: np.ndarray, mask: np.ndarray):
        self.preset = preset
        self.height = height
        self.mask = mask

    def build_chunks(self, chunk_size: int = 128):
        """
        Divide the world into square chunks of size chunk_size and generate .npy memmap files.
        Returns a dict mapping (cx, cz) to file paths.
        """
        size = self.preset.world_size
        max_height = int(self.height.max()) + 1
        chunks = {}

        for cx in range(0, size, chunk_size):
            for cz in range(0, size, chunk_size):
                xs = slice(cx, min(cx + chunk_size, size))
                zs = slice(cz, min(cz + chunk_size, size))

                x_len = xs.stop - xs.start
                z_len = zs.stop - zs.start

                # Ensure directory exists
                out_dir = Path("data/chunks")
                out_dir.mkdir(parents=True, exist_ok=True)

                path = out_dir / f"chunk_{cx}_{cz}.npy"
                mmap = np.memmap(path, shape=(x_len, max_height, z_len), dtype=np.uint8, mode="w+")

                # Vectorized fill
                h_slice = self.height[xs, zs]
                m_slice = self.mask[xs, zs]
                ys = np.arange(max_height)[:, np.newaxis, np.newaxis]  # Broadcast y
                below_surface = ys < h_slice[np.newaxis, :, :]  # shape (y, x, z)
                # Align mask axes to (x, y, z) to match voxel shape
                cond = below_surface.transpose(1, 0, 2)    # shape (x, y, z)
                # Simple block types: 1=stone, vary by biome (e.g., + biome_id)
                # Repeat biome IDs along the y-axis
                block_types = np.ones_like(mmap) * (1 + m_slice[:, np.newaxis, :])  # Example: biome_id as offset
                mmap[:] = np.where(cond, block_types, 0)  # 0=air

                # Handle sea level (fill water=2 below sea_level where height < sea_level)
                below_sea = (ys < self.preset.sea_level) & (ys >= h_slice[np.newaxis, :, :])  # shape (y,x,z)
                # Transpose to align with voxel axes (x,y,z)
                water_mask = below_sea.transpose(1, 0, 2)  # shape (x,y,z)
                mmap[water_mask] = 2  # Water block

                # Create per-voxel color memmap based on biome colors and water
                colors = np.array([b.color for b in self.preset.biomes], dtype=np.uint8)
                color_path = out_dir / f"chunk_{cx}_{cz}_color.npy"
                color_mmap = np.memmap(color_path, shape=(x_len, max_height, z_len, 3), dtype=np.uint8, mode="w+")
                # Build base RGB volume: broadcast biome colors per column
                color2d = colors[m_slice]  # (x_len, z_len, 3)
                color_vol = np.broadcast_to(color2d[:, np.newaxis, :, :], (x_len, max_height, z_len, 3)).copy()
                # Overwrite water voxels with blue
                water_color = np.array([0, 0, 255], dtype=np.uint8)
                water_mask = np.transpose(below_sea, (1, 0, 2))  # (x, y, z)
                color_vol[water_mask] = water_color
                color_mmap[:] = color_vol

                chunks[(cx, cz)] = (path, color_path)

        return chunks 