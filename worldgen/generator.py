from .config_schema import Preset
from .biome import BiomeMask
from .heightmap import HeightField
from .voxelizer import Voxelizer
from .api import _REGISTRY
from .trace import CallTrace
from pathlib import Path

def start_world(preset: Preset, preset_id: str) -> Path:
    """
    Orchestrates the entire world generation process from a preset.

    Args:
        preset: The Preset object defining the world.
        preset_id: A unique identifier for this world generation run.

    Returns:
        The path to the trace file, which contains the log of function calls.
    """
    trace = CallTrace()

    # 1. Generate core data
    mask = BiomeMask(preset).build()
    height = HeightField(preset, mask).build()
    vox_chunks = Voxelizer(preset, height, mask).build_chunks()
    trace.record("build_chunks", {"chunk_paths": [str(p) for p in vox_chunks.values()]})


    # 2. Apply registered function calls from preset
    for call in preset.calls:
        fn_name = call.get("fn")
        if fn_name in _REGISTRY:
            fn = _REGISTRY[fn_name]
            args = call.get("args", {})
            # Note: Here we would pass the voxel data to the function
            # e.g., fn(vox_chunks, **args)
            trace.record(fn_name, args)

    # 3. Save trace
    trace_dir = Path("data/traces")
    trace_dir.mkdir(exist_ok=True)
    trace_path = trace_dir / f"{preset_id}.trace.json"
    trace.save(str(trace_path))

    return trace_path 