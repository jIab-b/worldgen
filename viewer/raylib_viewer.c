/* viewer/raylib_viewer.c */
#include "raylib.h"
#include "raymath.h"
#include <emscripten/fetch.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[50];
    // Add other fields as needed from Preset
} Preset;

void fetchWorldData() {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.requestData = "{\"name\": \"default_world\"}";  // Customize based on Preset
    attr.requestHeaders = "Content-Type: application/json";
    attr.onload = [](emscripten_fetch_t *fetch) {
        if (fetch->status == 200) {
            printf("World data fetched: %s\n", fetch->data);
            // Parse and use data for rendering
        } else {
            printf("Fetch error: %d\n", fetch->status);
        }
        emscripten_fetch_close(fetch);
    };
    emscripten_fetch(&attr, "http://localhost:8000/generate");
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "WorldGen Viewer with Raylib");
    SetTargetFPS(60);

    fetchWorldData();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCube((Vector3){0, 0, 0}, 1, 1, 1, RED);  // Placeholder rendering
        EndDrawing();
    }

    CloseWindow();
    return 0;
} 