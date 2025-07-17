import cupy as cp  # type: ignore
import numpy as np
from numba import njit, prange
import random

@njit
def _hash(seed: int, x: int, y: int) -> float:
    """Simple deterministic hash for noise grid."""
    h = seed ^ (x * 0x5DEECE66D + y * 0xBB67AE85)  # LCG-inspired
    return (h % 100000) / 100000.0  # Normalize to [0,1]

def value_noise2D(seed: int, xs: cp.ndarray, zs: cp.ndarray) -> cp.ndarray:
    """2D value noise on GPU with bilinear interpolation."""
    x_floor = cp.floor(xs).astype(int)
    z_floor = cp.floor(zs).astype(int)
    x_frac = xs - x_floor
    z_frac = zs - z_floor

    # Grid values
    v00 = cp.array([_hash(seed, x, z) for x, z in zip(x_floor.get(), z_floor.get())])  # TODO: Vectorize fully on GPU if slow
    v01 = cp.array([_hash(seed, x, z+1) for x, z in zip(x_floor.get(), z_floor.get())])
    v10 = cp.array([_hash(seed, x+1, z) for x, z in zip(x_floor.get(), z_floor.get())])
    v11 = cp.array([_hash(seed, x+1, z+1) for x, z in zip(x_floor.get(), z_floor.get())])

    # Bilinear interp
    top = v00 + x_frac * (v10 - v00)
    bottom = v01 + x_frac * (v11 - v01)
    return top + z_frac * (bottom - top)

def fbm_2D(seed: int, xs: cp.ndarray, zs: cp.ndarray, freq: float, octaves: int = 5) -> cp.ndarray:
    """Fractional Brownian motion by summing octaves."""
    # Initialize as float to accumulate fractional noise without casting errors
    noise = cp.zeros_like(xs, dtype=cp.float64)
    amp = 1.0
    for o in range(octaves):
        noise += amp * value_noise2D(seed + o, xs * freq, zs * freq)
        freq *= 2.0
        amp *= 0.5
    return noise / (2 - 1 / (1 << (octaves - 1)))  # Normalize

def value_noise2D_cpu(seed: int, xs: np.ndarray, zs: np.ndarray) -> np.ndarray:
    """CPU fallback implementation of value noise."""
    result = np.zeros_like(xs, dtype=float)
    for i in prange(xs.size):
        x, z = xs.flat[i], zs.flat[i]
        x_floor, z_floor = int(x), int(z)
        x_frac, z_frac = x - x_floor, z - z_floor

        v00 = _hash(seed, x_floor, z_floor)
        v01 = _hash(seed, x_floor, z_floor + 1)
        v10 = _hash(seed, x_floor + 1, z_floor)
        v11 = _hash(seed, x_floor + 1, z_floor + 1)

        top = v00 + x_frac * (v10 - v00)
        bottom = v01 + x_frac * (v11 - v01)
        result.flat[i] = top + z_frac * (bottom - top)
    return result

def fbm_2D_cpu(seed: int, xs: np.ndarray, zs: np.ndarray, freq: float, octaves: int) -> np.ndarray:
    noise = np.zeros_like(xs, dtype=float)
    amp = 1.0
    current_freq = freq
    for o in range(octaves):
        noise += amp * value_noise2D_cpu(seed + o, xs * current_freq, zs * current_freq)
        current_freq *= 2.0
        amp *= 0.5
    return noise / (2 - 1 / (1 << (octaves - 1))) 