#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

#define MAX_ASTEROIDS 100

// Game State
typedef struct {
    Vector3 position;
    Model model;
    float radius;
} Asteroid;

static Asteroid asteroids[MAX_ASTEROIDS];
static int asteroidCount = 0;
static bool gameOver = false;
static float score = 0;

// Helpers
static Model Load3DModelWithScale(const char *file, float scale) {
    Model m = LoadModel(file);
    m.transform = MatrixScale(scale, scale, scale);
    return m;
}
/*
// Load Skybox Cubemap
static Cubemap LoadSkybox() {
    Texture2D skyRight  = LoadTexture("resources/skybox_right.png");
    Texture2D skyLeft   = LoadTexture("resources/skybox_left.png");
    Texture2D skyTop    = LoadTexture("resources/skybox_top.png");
    Texture2D skyBottom = LoadTexture("resources/skybox_right.png");
    Texture2D skyBack   = LoadTexture("resources/skybox_left.png");
    Texture2D skyFront  = LoadTexture("resources/skybox_top.png");
    Cubemap cubemap = { skyRight, skyLeft, skyTop, skyBottom, skyBack, skyFront };
    return cubemap;
}
*/
int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Raylib 3D Spaceship Game");
    SetTargetFPS(60);

    // Camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 5.0f, -15.0f };
    camera.target   = (Vector3){ 0.0f, 0.0f,  0.0f };
    camera.up       = (Vector3){ 0.0f, 1.0f,  0.0f };
    camera.fovy     = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load Models
    Model shipModel = Load3DModelWithScale("resources/spaceship.obj", 0.5f);
    Model asteroidModel = Load3DModelWithScale("resources/asteroid.obj", 0.5f);

    // Player
    Vector3 shipPos = {0.0f, 0.0f, 0.0f};
    float shipSpeed = 0.2f;
    float shipRadius = 1.0f;

 //   Cubemap skyCubemap = LoadSkybox();

    // Main loop
    while (!WindowShouldClose()) {

        // --- UPDATE --
        if (!gameOver) {
            score += GetFrameTime() * 10.0f;

            // Player Movement
            if (IsKeyDown(KEY_LEFT))  shipPos.x -= shipSpeed;
            if (IsKeyDown(KEY_RIGHT)) shipPos.x += shipSpeed;
            if (IsKeyDown(KEY_UP))    shipPos.y += shipSpeed;
            if (IsKeyDown(KEY_DOWN))  shipPos.y -= shipSpeed;

            // Spawn asteroids
            if (asteroidCount < MAX_ASTEROIDS && GetRandomValue(0,100) < 2) {
                Asteroid a;
                float spawnX = GetRandomValue(-10,10);
                float spawnY = GetRandomValue(-5,5);
                a.position = (Vector3){ spawnX, spawnY, 30.0f };
                a.model = asteroidModel;
                a.radius = 1.0f;
                asteroids[asteroidCount++] = a;
            }

            // Update asteroids
            for (int i=0; i<asteroidCount; i++) {
                asteroids[i].position.z -= 0.15f + GetRandomValue(0,3)/10.0f;
            }

            // Remove passed asteroids
            int newCount = 0;
            for (int i=0; i<asteroidCount; i++) {
                if (asteroids[i].position.z > -5.0f) {
                    asteroids[newCount++] = asteroids[i];
                }
            }
            asteroidCount = newCount;

            // Collision check
            for (int i=0; i<asteroidCount; i++) {
                if (CheckCollisionSpheres(shipPos, shipRadius, asteroids[i].position, asteroids[i].radius)) {
                    gameOver = true;
                }
            }
        } else {
            if (IsKeyPressed(KEY_R)) {
                gameOver = false;
                score = 0;
                asteroidCount = 0;
            }
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

        // Draw Skybox
        //DrawSkybox(skyCubemap, (Vector3){ camera.position.x, camera.position.y, camera.position.z });

        // Draw Ship
        DrawModel(shipModel, shipPos, 1.0f, WHITE);

        // Draw Asteroids
        for (int i=0; i<asteroidCount; i++) {
            DrawModel(asteroids[i].model, asteroids[i].position, 1.0f, GRAY);
        }

        EndMode3D();

        //DrawText(FormatText("Score: %d", (int)score), 10, 10, 20, WHITE);

        if (gameOver) DrawText("GAME OVER! Press R to restart", 300, 300, 30, RED);

        EndDrawing();
    }

    UnloadModel(shipModel);
    UnloadModel(asteroidModel);
    CloseWindow();

    return 0;
}
