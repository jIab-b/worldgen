#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
// Mesh loader globals
static float*    verts     = NULL;
static unsigned int* inds   = NULL;
static int       vCount    = 0;
static int       iCount    = 0;
static Mesh      mesh      = { 0 };
static int       chunkX    = 0;
static int       chunkZ    = 0;
// Simple load error flag and message
static bool      loadError = false;
static char      errorMsg[128] = { 0 };

// Load mesh chunk from interleaved vertex/index binaries
static void loadChunkMesh(int cx, int cz)
{
    // Reset any previous error
    loadError = false;
    // Unload previously uploaded mesh
    if (mesh.vertexCount > 0) UnloadMesh(mesh);
    // Free previous data
    if (verts) UnloadFileData((unsigned char*)verts);
    if (inds)  UnloadFileData((unsigned char*)inds);

    unsigned int sizeV = 0;
    unsigned char *blobV = LoadFileData(TextFormat("chunks/chunk_%d_%d_vertices.bin", cx, cz), &sizeV);
    if (!blobV || sizeV == 0) {
        loadError = true;
        sprintf(errorMsg, "Error loading verts for chunk %d,%d", cx, cz);
        return;
    }
    vCount = sizeV / sizeof(float);
    verts = (float*)blobV;

    unsigned int sizeI = 0;
    unsigned char *blobI = LoadFileData(TextFormat("chunks/chunk_%d_%d_indices.bin", cx, cz), &sizeI);
    if (!blobI || sizeI == 0) {
        loadError = true;
        sprintf(errorMsg, "Error loading indices for chunk %d,%d", cx, cz);
        return;
    }
    iCount = sizeI / sizeof(unsigned int);
    inds = (unsigned int*)blobI;

    mesh.vertexCount   = vCount/8;
    mesh.triangleCount = iCount/3;
    mesh.vertices      = verts;
    mesh.normals       = verts + 3;
    mesh.texcoords     = verts + 6;
    mesh.indices       = (unsigned short*)inds;
    UploadMesh(&mesh, false);

    chunkX = cx;
    chunkZ = cz;
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib mesh viewer");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target   = (Vector3){  0.0f,  0.0f,  0.0f };
    camera.up       = (Vector3){  0.0f,  1.0f,  0.0f };
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);
    // Load default material for mesh rendering
    Material material = LoadMaterialDefault();
    //--------------------------------------------------------------------------------------

    // Load initial mesh chunk at (0,0)
    loadChunkMesh(0, 0);

    // Main loop
    while (!WindowShouldClose())
    {
        // Update camera (floating) with Shift×10 speed
        int updates = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 10 : 1;
        for (int i = 0; i < updates; i++) UpdateCamera(&camera, CAMERA_FREE);

        // Chunk reload controls (arrow keys and R)
        if (IsKeyPressed(KEY_UP))    loadChunkMesh(chunkX,     chunkZ + 1);
        if (IsKeyPressed(KEY_DOWN))  loadChunkMesh(chunkX,     chunkZ - 1);
        if (IsKeyPressed(KEY_RIGHT)) loadChunkMesh(chunkX + 1, chunkZ    );
        if (IsKeyPressed(KEY_LEFT))  loadChunkMesh(chunkX - 1, chunkZ    );
        if (IsKeyPressed(KEY_R))     loadChunkMesh(chunkX,     chunkZ    );

        // Draw scene (or error)
        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (loadError) {
                DrawRectangle(10, 10, 340, 50, Fade(RED, 0.8f));
                DrawText(errorMsg, 20, 20, 10, RED);
                DrawText("Press R to retry", 20, 35, 10, RED);
                EndDrawing();
                continue;
            }

            BeginMode3D(camera);
                // Draw the mesh with default material
                DrawMesh(mesh, material, MatrixIdentity());
                DrawGrid(10, 1.0f);
            EndMode3D();

            // HUD overlay
            DrawRectangle(10, 10, 260, 50, Fade(SKYBLUE, 0.5f));
            DrawText(TextFormat("FPS: %03i  Chunk: %02i,%02i", GetFPS(), chunkX, chunkZ), 20, 20, 10, BLACK);
            DrawText(TextFormat("Pos: %.2f,%.2f,%.2f", camera.position.x, camera.position.y, camera.position.z), 20, 35, 10, BLACK);

        EndDrawing();
    }

    // De-initialization
    UnloadMesh(mesh);
    UnloadMaterial(material);
    CloseWindow();
    return 0;
}