import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import uuid

from worldgen.config_schema import Preset
from worldgen.generator import start_world
from worldgen.api import _SCHEMA as api_schema

app = FastAPI(
    title="Text-to-Voxel World Generation API",
    description="An API for generating voxel worlds from JSON presets.",
    version="0.1.0",
)

# Allow CORS for the viewer to fetch data
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # Allow all origins for simplicity
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Serve the data directory statically
app.mount("/data", StaticFiles(directory="data"), name="data")

class GenerateResponse(BaseModel):
    preset_id: str
    message: str
    trace_path: str

@app.post("/generate", response_model=GenerateResponse)
async def generate_world(preset: Preset):
    """
    Accepts a world preset, generates the world data, and returns a trace file path.
    """
    try:
        preset_id = str(uuid.uuid4())
        trace_path = start_world(preset, preset_id)
        return {
            "preset_id": preset_id,
            "message": "World generation initiated successfully.",
            "trace_path": str(trace_path),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/schema")
async def get_master_schema():
    """
    Returns the aggregated JSON Schema for the Preset and all registered API functions.
    This is useful for ML model integration.
    """
    return {
        "preset_schema": Preset.model_json_schema(),
        "functions_schema": api_schema,
    }

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000) 