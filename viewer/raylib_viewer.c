#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
// Add chunk dimensions and data pointers
#define CHUNK_SIZE_X 128
#define CHUNK_SIZE_Z 128
#define MAX_HEIGHT    70  // adjust to your world’s max height

static unsigned char *blocks = NULL;
static unsigned char *colors = NULL;

// Load raw chunk data for chunk (cx,cz)
static void loadChunk(int cx, int cz)
{
    if (blocks) UnloadFileData(blocks);
    if (colors) UnloadFileData(colors);
    char path1[64], path2[64];
    snprintf(path1, sizeof(path1), "chunks/chunk_%d_%d.raw", cx, cz);
    snprintf(path2, sizeof(path2), "chunks/chunk_%d_%d_color.raw", cx, cz);
    int size1 = 0, size2 = 0;
    blocks = LoadFileData(path1, &size1);
    colors = LoadFileData(path2, &size2);
}

// Draw the loaded chunk
static void drawChunk(void)
{
    if (!blocks || !colors) return;
    for (int x = 0; x < CHUNK_SIZE_X; x++)
    for (int z = 0; z < CHUNK_SIZE_Z; z++)
    for (int y = 0; y < MAX_HEIGHT;    y++)
    {
        int idx = x*MAX_HEIGHT*CHUNK_SIZE_Z + y*CHUNK_SIZE_Z + z;
        unsigned char b = blocks[idx];
        if (b == 0) continue;  // air
        Color col = { colors[idx*3+0], colors[idx*3+1], colors[idx*3+2], 255 };
        DrawCube((Vector3){ (float)x, (float)y, (float)z }, 1,1,1, col);
    }
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera free");

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    DisableCursor();                    // Limit cursor to relative movement inside the window

    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Load the initial chunk at (0,0)
    loadChunk(0, 0);

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_FREE);

        if (IsKeyPressed('Z')) camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // Draw our voxel chunk instead of a single cube
            BeginMode3D(camera);
                drawChunk();
                DrawGrid(10, 1.0f);
            EndMode3D();

            DrawRectangle( 10, 10, 320, 93, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines( 10, 10, 320, 93, BLUE);

            DrawText("Free camera default controls:", 20, 20, 10, BLACK);
            DrawText("- Mouse Wheel to Zoom in-out", 40, 40, 10, DARKGRAY);
            DrawText("- Mouse Wheel Pressed to Pan", 40, 60, 10, DARKGRAY);
            DrawText("- Z to zoom to (0, 0, 0)", 40, 80, 10, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}