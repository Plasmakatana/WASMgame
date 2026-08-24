#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#define MAX_ASTEROIDS 100

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

// ============================================================
// SCALE SETTINGS
// ============================================================

// Spaceship scale
#define SHIP_SCALE 0.5f

// IMPORTANT:
// Your asteroid model may be huge.
// Start VERY small.
#define ASTEROID_SCALE 0.001f

// Collision sizes
#define SHIP_RADIUS     1.2f
#define ASTEROID_RADIUS 0.6f

// ============================================================
// PBR
// ============================================================

#define GLSL_VERSION 100
#define MAX_LIGHTS 4

#define LIGHT_DIRECTIONAL 4
#define LIGHT_POINT       0

// ============================================================
// LIGHT
// ============================================================

typedef struct
{
    int enabled;
    int type;

    Vector3 position;
    Vector3 target;

    float color[4];
    float intensity;

    int enabledLoc;
    int typeLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int intensityLoc;

} GameLight;

// ============================================================
// ASTEROID
// ============================================================

typedef struct
{
    Vector3 position;

    float speed;

    float rotationAngle;
    float rotationSpeed;

    Vector3 rotationAxis;

} Asteroid;
typedef struct {
    Vector3 position;
    float speed;
    bool present;
} Particle;
// ============================================================
// GLOBAL GAME STATE
// ============================================================

static Asteroid asteroids[MAX_ASTEROIDS];
static Particle particles[100] = {(Vector3){0.0f,0.0f,0.0f},30.0f,false};
static int asteroidCount = 0;

static bool gameOver = false;

static float score = 0.0f;

// ============================================================
// LIGHT CREATION
// ============================================================
float Signum(float x) {
    if (x > 0.0f) return 1.0f;
    if (x < 0.0f) return -1.0f;
    return 0.0f;
}
static GameLight CreateGameLight(
    Shader shader,
    int index,
    int type,
    Vector3 position,
    Vector3 target,
    Color color,
    float intensity
)
{
    GameLight light = { 0 };

    light.enabled = 1;
    light.type = type;

    light.position = position;
    light.target = target;

    light.color[0] = (float)color.r / 255.0f;
    light.color[1] = (float)color.g / 255.0f;
    light.color[2] = (float)color.b / 255.0f;
    light.color[3] = (float)color.a / 255.0f;

    light.intensity = intensity;

    light.enabledLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].enabled",
                index
            )
        );

    light.typeLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].type",
                index
            )
        );

    light.positionLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].position",
                index
            )
        );

    light.targetLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].target",
                index
            )
        );

    light.colorLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].color",
                index
            )
        );

    light.intensityLoc =
        GetShaderLocation(
            shader,
            TextFormat(
                "lights[%i].intensity",
                index
            )
        );

    return light;
}

// ============================================================
// UPDATE LIGHT
// ============================================================

static void UpdateGameLight(
    Shader shader,
    GameLight light
)
{
    SetShaderValue(
        shader,
        light.enabledLoc,
        &light.enabled,
        SHADER_UNIFORM_INT
    );

    SetShaderValue(
        shader,
        light.typeLoc,
        &light.type,
        SHADER_UNIFORM_INT
    );

    float position[3] =
    {
        light.position.x,
        light.position.y,
        light.position.z
    };

    SetShaderValue(
        shader,
        light.positionLoc,
        position,
        SHADER_UNIFORM_VEC3
    );

    float target[3] =
    {
        light.target.x,
        light.target.y,
        light.target.z
    };

    SetShaderValue(
        shader,
        light.targetLoc,
        target,
        SHADER_UNIFORM_VEC3
    );

    SetShaderValue(
        shader,
        light.colorLoc,
        light.color,
        SHADER_UNIFORM_VEC4
    );

    SetShaderValue(
        shader,
        light.intensityLoc,
        &light.intensity,
        SHADER_UNIFORM_FLOAT
    );
}

// ============================================================
// LOAD MODEL
// ============================================================

static Model LoadGameModel(const char *filename)
{
    Model model = LoadModel(filename);

    if (!IsModelValid(model))
    {
        TraceLog(
            LOG_ERROR,
            "FAILED TO LOAD MODEL: %s",
            filename
        );
    }
    else
    {
        TraceLog(
            LOG_INFO,
            "Loaded model: %s",
            filename
        );

        TraceLog(
            LOG_INFO,
            "Meshes: %d",
            model.meshCount
        );

        TraceLog(
            LOG_INFO,
            "Materials: %d",
            model.materialCount
        );
    }

    return model;
}

// ============================================================
// PRINT MODEL SIZE
// ============================================================

static void PrintModelBounds(
    const char *name,
    Model model
)
{
    if (!IsModelValid(model))
        return;

    BoundingBox bounds =
        GetModelBoundingBox(model);

    float width =
        bounds.max.x - bounds.min.x;

    float height =
        bounds.max.y - bounds.min.y;

    float depth =
        bounds.max.z - bounds.min.z;

    TraceLog(
        LOG_INFO,
        "%s bounds:",
        name
    );

    TraceLog(
        LOG_INFO,
        "  Min: %.3f %.3f %.3f",
        bounds.min.x,
        bounds.min.y,
        bounds.min.z
    );

    TraceLog(
        LOG_INFO,
        "  Max: %.3f %.3f %.3f",
        bounds.max.x,
        bounds.max.y,
        bounds.max.z
    );

    TraceLog(
        LOG_INFO,
        "  Size: %.3f x %.3f x %.3f",
        width,
        height,
        depth
    );
}

// ============================================================
// CREATE 1x1 DUMMY TEXTURE
//
// We deliberately USE the PBR texture pipeline.
//
// These textures are not bypasses.
// They are valid PBR input maps, just with one pixel.
// ============================================================

static Texture2D CreateDummyTexture(Color color)
{
    Image image =
        GenImageColor(
            1,
            1,
            color
        );

    Texture2D texture =
        LoadTextureFromImage(image);

    UnloadImage(image);

    return texture;
}

// ============================================================
// MAIN
// ============================================================
float velX=0;
float velY=0;
float accelX=0;
float accelY=0;
int main(void)
{
    // --------------------------------------------------------
    // Window
    // --------------------------------------------------------

    SetConfigFlags(
        FLAG_MSAA_4X_HINT
    );

    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Raylib 3D Spaceship Game"
    );

    SetTargetFPS(60);

    // --------------------------------------------------------
    // Camera
    // --------------------------------------------------------

    Camera3D camera = { 0 };

    camera.position =
        (Vector3)
        {
            0.0f,
            0.0f,
            -15.0f
        };

    camera.target =
        (Vector3)
        {
            0.0f,
            0.0f,
            0.0f
        };

    camera.up =
        (Vector3)
        {
            0.0f,
            1.0f,
            0.0f
        };

    camera.fovy = 60.0f;

    camera.projection =
        CAMERA_PERSPECTIVE;

    // ========================================================
    // LOAD PBR SHADER
    // ========================================================

    Shader pbrShader =
        LoadShader(
            TextFormat(
                "resources/shaders/glsl%i/pbr.vs",
                GLSL_VERSION
            ),
            TextFormat(
                "resources/shaders/glsl%i/pbr.fs",
                GLSL_VERSION
            )
        );

    // --------------------------------------------------------
    // Tell raylib where the PBR shader maps are
    //
    // This follows the official raylib PBR example.
    // --------------------------------------------------------

    pbrShader.locs[SHADER_LOC_MAP_ALBEDO] =
        GetShaderLocation(
            pbrShader,
            "albedoMap"
        );

    pbrShader.locs[SHADER_LOC_MAP_METALNESS] =
        GetShaderLocation(
            pbrShader,
            "mraMap"
        );

    pbrShader.locs[SHADER_LOC_MAP_NORMAL] =
        GetShaderLocation(
            pbrShader,
            "normalMap"
        );

    pbrShader.locs[SHADER_LOC_MAP_EMISSION] =
        GetShaderLocation(
            pbrShader,
            "emissiveMap"
        );

    pbrShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
        GetShaderLocation(
            pbrShader,
            "albedoColor"
        );

    pbrShader.locs[SHADER_LOC_VECTOR_VIEW] =
        GetShaderLocation(
            pbrShader,
            "viewPos"
        );

    // --------------------------------------------------------
    // Number of lights
    //
    // IMPORTANT:
    // The raylib example uses MAX_LIGHTS here.
    // --------------------------------------------------------

    int lightCountLoc =
        GetShaderLocation(
            pbrShader,
            "numOfLights"
        );

    int maxLightCount =
        MAX_LIGHTS;

    SetShaderValue(
        pbrShader,
        lightCountLoc,
        &maxLightCount,
        SHADER_UNIFORM_INT
    );

    // --------------------------------------------------------
    // Ambient
    //
    // Same concept as raylib's PBR example.
    // --------------------------------------------------------

    float ambientIntensity =
        0.001f;

    Color ambientColor =
        (Color)
        {
            36,
            52,
            200,
            100
        };

    Vector3 ambientColorNormalized =
        {
            ambientColor.r / 255.0f,
            ambientColor.g / 255.0f,
            ambientColor.b / 255.0f
        };

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "ambientColor"
        ),
        &ambientColorNormalized,
        SHADER_UNIFORM_VEC3
    );

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "ambient"
        ),
        &ambientIntensity,
        SHADER_UNIFORM_FLOAT
    );

    // --------------------------------------------------------
    // Runtime material uniform locations
    //
    // These are exactly the kind of values the official
    // example changes before drawing each material.
    // --------------------------------------------------------

    int metallicValueLoc =
        GetShaderLocation(
            pbrShader,
            "metallicValue"
        );

    int roughnessValueLoc =
        GetShaderLocation(
            pbrShader,
            "roughnessValue"
        );

    int emissiveIntensityLoc =
        GetShaderLocation(
            pbrShader,
            "emissivePower"
        );

    int emissiveColorLoc =
        GetShaderLocation(
            pbrShader,
            "emissiveColor"
        );

    int textureTilingLoc =
        GetShaderLocation(
            pbrShader,
            "tiling"
        );

    // ========================================================
    // DUMMY PBR TEXTURES
    //
    // ALBEDO:
    // white so material albedo color controls the actual color.
    //
    // MRA:
    // R = metallic
    // G = roughness
    // B = AO
    //
    // NORMAL:
    // flat tangent-space normal = (128,128,255)
    //
    // EMISSIVE:
    // black, so nothing emits light.
    // ========================================================

    Texture2D dummyAlbedo =
        CreateDummyTexture(
            WHITE
        );

    Texture2D dummyMRA =
        CreateDummyTexture(
            (Color)
            {
                0,
                0,
                255,
                255
            }
        );

    Texture2D dummyNormal =
        CreateDummyTexture(
            (Color)
            {
                128,
                128,
                255,
                255
            }
        );

    Texture2D dummyEmissive =
        CreateDummyTexture(
            WHITE
        );

    // ========================================================
    // SKY LIGHT
    // ========================================================

    GameLight skyLight =
        CreateGameLight(
            pbrShader,
            0,
            LIGHT_DIRECTIONAL,

            (Vector3)
            {
                -2.0f,
                12.0f,
                40.0f
            },

            (Vector3)
            {
                0.0f,
                0.0f,
                0.0f
            },

            (Color)
            {
                75,
                150,
                255,
                255
            },

            200.0f
        );
        GameLight shipLight1 =
        CreateGameLight(
            pbrShader,
            1,
            LIGHT_DIRECTIONAL,

            (Vector3)
            {
                10.0f,
                10.0f,
                -10.0f
            },

            (Vector3)
            {
                0.0f,
                0.0f,
                0.0f
            },

            (Color){255,000,000,255},

            10.0f
        );
        GameLight shipLight2 =
        CreateGameLight(
            pbrShader,
            2,
            LIGHT_DIRECTIONAL,

            (Vector3)
            {
                -10.0f,
                -10.0f,
                -10.0f
            },

            (Vector3)
            {
                0.0f,
                0.0f,
                0.0f
            },

            (Color){050,000,255,255},

            20.0f
        );
        GameLight shipLight =
        CreateGameLight(
            pbrShader,
            3,
            LIGHT_DIRECTIONAL,

            (Vector3)
            {
                0.0f,
                0.0f,
                2.0f
            },

            (Vector3)
            {
                0.0f,
                0.0f,
                10.0f
            },

            (Color){200,200,255,255},

            30.0f
        );

    // ========================================================
    // LOAD MODELS
    // ========================================================

    Model shipModel =
        LoadGameModel(
            "resources/spaceship.obj"
        );

    Model asteroidModel =
        LoadGameModel(
            "resources/asteroid.obj"
        );

    PrintModelBounds(
        "SPACESHIP",
        shipModel
    );

    PrintModelBounds(
        "ASTEROID",
        asteroidModel
    );

    // ========================================================
    // LOAD SKY
    // ========================================================

    Texture2D skyTexture =
        LoadTexture(
            "resources/sky.png"
        );

    if (!IsTextureValid(skyTexture))
    {
        TraceLog(
            LOG_ERROR,
            "FAILED TO LOAD sky.png"
        );
    }
    else
    {
        TraceLog(
            LOG_INFO,
            "Sky texture loaded: %d x %d",
            skyTexture.width,
            skyTexture.height
        );
    }

    // ========================================================
    // SHIP MATERIAL
    // ========================================================

    if (IsModelValid(shipModel))
    {
        for (
            int i = 0;
            i < shipModel.materialCount;
            i++
        )
        {
            Material *material =
                &shipModel.materials[i];

            // Assign PBR shader
            material->shader =
                pbrShader;

            // Dummy maps
            material->maps[MATERIAL_MAP_ALBEDO]
                .texture =
                dummyAlbedo;

            material->maps[MATERIAL_MAP_METALNESS]
                .texture =
                dummyMRA;

            material->maps[MATERIAL_MAP_NORMAL]
                .texture =
                dummyNormal;

            material->maps[MATERIAL_MAP_EMISSION]
                .texture =
                dummyEmissive;

            // Ship color
            material->maps[MATERIAL_MAP_ALBEDO]
                .color =
                (Color)
                {
                    200,
                    220,
                    255,
                    255
                };

            // Ship PBR values
            material->maps[MATERIAL_MAP_METALNESS]
                .value =
                0.7f;

            material->maps[MATERIAL_MAP_ROUGHNESS]
                .value =
                0.5f;

            material->maps[MATERIAL_MAP_OCCLUSION]
                .value =
                1.0f;

            material->maps[MATERIAL_MAP_EMISSION]
                .color =
                WHITE;
        }
    }

    // ========================================================
    // ASTEROID MATERIAL
    // ========================================================

    if (IsModelValid(asteroidModel))
    {
        for (
            int i = 0;
            i < asteroidModel.materialCount;
            i++
        )
        {
            Material *material =
                &asteroidModel.materials[i];

            // Assign PBR shader
            material->shader =
                pbrShader;

            // Dummy maps
            material->maps[MATERIAL_MAP_ALBEDO]
                .texture =
                dummyAlbedo;

            material->maps[MATERIAL_MAP_METALNESS]
                .texture =
                dummyMRA;

            material->maps[MATERIAL_MAP_NORMAL]
                .texture =
                dummyNormal;

            material->maps[MATERIAL_MAP_EMISSION]
                .texture =
                dummyEmissive;

            // Asteroid color
            material->maps[MATERIAL_MAP_ALBEDO]
                .color =
                (Color)
                {
                    035,
                    025,
                    030,
                    255
                };

            // Asteroid PBR values
            material->maps[MATERIAL_MAP_METALNESS]
                .value =
                0.2f;

            material->maps[MATERIAL_MAP_ROUGHNESS]
                .value =
                0.7f;

            material->maps[MATERIAL_MAP_OCCLUSION]
                .value =
                1.0f;

            material->maps[MATERIAL_MAP_EMISSION]
                .color =
                ORANGE;
        }
    }

    // ========================================================
    // ENABLE ALL PBR TEXTURE PATHS
    //
    // THIS IS IMPORTANT.
    //
    // We are NOT disabling texture processing.
    // We supply valid dummy textures instead.
    // ========================================================

    int usage =
        1;

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "useTexAlbedo"
        ),
        &usage,
        SHADER_UNIFORM_INT
    );

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "useTexNormal"
        ),
        &usage,
        SHADER_UNIFORM_INT
    );

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "useTexMRA"
        ),
        &usage,
        SHADER_UNIFORM_INT
    );

    SetShaderValue(
        pbrShader,
        GetShaderLocation(
            pbrShader,
            "useTexEmissive"
        ),
        &usage,
        SHADER_UNIFORM_INT
    );

    // ========================================================
    // TEXTURE TILING
    //
    // Same approach as raylib's PBR example.
    // ========================================================

    Vector2 shipTextureTiling =
        {
            1.0f,
            1.0f
        };

    Vector2 asteroidTextureTiling =
        {
            1.0f,
            1.0f
        };

    // ========================================================
    // PLAYER
    // ========================================================

    Vector3 shipPos =
        {
            0.0f,
            0.0f,
            0.0f
        };

    // ========================================================
    // MAIN LOOP
    // ========================================================
    float counter = 0;
    float shakeTrauma = 0.8f;
    const float positionShakeStrength = 0.3f;  // Max offset for position
    const float targetShakeStrength = 0.3f;
    Vector3 originalPosition = camera.position;
    Vector3 originalTarget = camera.target;
    
    RenderTexture2D gameTarget = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    Shader bloomShader = LoadShader(0, "resources/shaders/bloom.fs");

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        counter+=0.0001f;
        Clamp(shakeTrauma,0.0f,1.0f);    
        

        // ====================================================
        // UPDATE LIGHTING
        // ====================================================

        float cameraPos[3] =
        {
            camera.position.x,
            camera.position.y,
            camera.position.z
        };

        SetShaderValue(
            pbrShader,
            pbrShader.locs[SHADER_LOC_VECTOR_VIEW],
            cameraPos,
            SHADER_UNIFORM_VEC3
        );

        UpdateGameLight(
            pbrShader,
            skyLight
        );
        UpdateGameLight(
            pbrShader,
            shipLight1
        );
        UpdateGameLight(
            pbrShader,
            shipLight2
        );
        UpdateGameLight(
            pbrShader,
            shipLight
        );

        // ====================================================
        // GAME UPDATE
        // ====================================================

        if (!gameOver)
        {
            // ------------------------------------------------
            // Score
            // ------------------------------------------------

            score +=
                dt * 10.0f;

            // ------------------------------------------------
            // Player movement
            // ------------------------------------------------

            float moveSpeed =
                8.0f;
            float movement =
                moveSpeed * dt;
            
            Vector3 clickPosition;
            bool hasTarget = false;
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                Ray ray = GetMouseRay(GetMousePosition(), camera);
                if (fabs(ray.direction.z) > 0.0001f)
                {
                    float t = -ray.position.z / ray.direction.z;

                    if (t >= 0.0f)
                        {
                            clickPosition = Vector3Add(
                                ray.position,
                                Vector3Scale(ray.direction, t)
                            );

                            hasTarget = true;
                        }
                        hasTarget=false;
                }
                
                accelX=Signum(clickPosition.x-shipPos.x)*0.008f;
                accelY=Signum(clickPosition.y-shipPos.y)*0.008f;                 
            }
            else{accelX/=1.1f;accelY/=1.1f;}
            if (
                IsKeyDown(KEY_LEFT) ||
                IsKeyDown(KEY_A)
            )
            {
                accelX=0.005f;
                //shipPos.x +=
                  //  movement;
            }

            if (
                IsKeyDown(KEY_RIGHT) ||
                IsKeyDown(KEY_D)
            )
            {
                accelX=-0.005f;
                //shipPos.x -=
                  //  movement;
            }

            if (
                IsKeyDown(KEY_UP) ||
                IsKeyDown(KEY_W)
            )
            {
                accelY=0.005f;
//                shipPos.y +=
  //                  movement;
            }

            if (
                IsKeyDown(KEY_DOWN) ||
                IsKeyDown(KEY_S)
            )
            {
                accelY=-0.005f;
                //shipPos.y -=
                  //  movement;
            }
            
            velX+=accelX;
            velY+=accelY;
            velX=Clamp(velX,-0.5f,0.5f);
            velY=Clamp(velY,-0.5f,0.5f);

            shipPos.x+=velX;
            shipPos.y+=velY;
            shipLight.position=(Vector3){shipPos.x,shipPos.y,shipPos.z+2.0f};
            
            // ------------------------------------------------
            // Player bounds
            // ------------------------------------------------

            shipPos.x =
                Clamp(
                    shipPos.x,
                    -18.0f,
                    18.0f
                );

            shipPos.y =
                Clamp(
                    shipPos.y,
                    -9.0f,
                    9.0f
                );

            // ------------------------------------------------
            // Spawn asteroids
            // ------------------------------------------------
            
            
            
            if (IsKeyDown(KEY_LEFT_SHIFT))
            {
            camera.fovy+=0.5f;if(camera.fovy>90.0f)camera.fovy=90.0f;if(camera.fovy<60.0f)camera.fovy=60.0f;
            shakeTrauma=0.5f;
            }
            else
            {
            // Reset to original transform
            camera.fovy-=1.0f;if(camera.fovy>90.0f)camera.fovy=90.0f;if(camera.fovy<60.0f)camera.fovy=60.0f;
            shakeTrauma=0.3f;
            }
            float shakePower = shakeTrauma * shakeTrauma; // Non-linear falloff

            // Random offset for position
            float offsetX = (GetRandomValue(-100, 100) / 100.0f) * shakePower * positionShakeStrength;
            float offsetY = (GetRandomValue(-100, 100) / 100.0f) * shakePower * positionShakeStrength;

            // Random offset for target (creates "aim jitter")
            float targetX = (GetRandomValue(-100, 100) / 100.0f) * shakePower * targetShakeStrength;
            float targetY = (GetRandomValue(-100, 100) / 100.0f) * shakePower * targetShakeStrength;
            
            camera.position=(Vector3){shipPos.x/10+offsetX,shipPos.y/10+offsetY,originalPosition.z};

            camera.target=(Vector3){shipPos.x/5+ targetX,shipPos.y/5+ targetY,shipPos.z};
            if(velX>0&&(!IsKeyDown(KEY_A))&&(!IsKeyDown(KEY_LEFT))){velX-=0.005;}
            else if(velX<0&&(!IsKeyDown(KEY_D))&&(!IsKeyDown(KEY_RIGHT))){velX+=0.005;}
            if(velY>0&&(!IsKeyDown(KEY_W))&&(!IsKeyDown(KEY_UP))){velY-=0.005;}
            else if((!IsKeyDown(KEY_S))&&(!IsKeyDown(KEY_DOWN))){velY+=0.005;}
            if(GetRandomValue(0,100)<70){
                Particle particle = {0};
                particle.position=(Vector3){(float)GetRandomValue(-20,20),(float)GetRandomValue(-10,10),100.0f};
                particle.speed = 30.0f + (float)GetRandomValue(0.0f,30.0f);
                int idx = (int)GetRandomValue(0,99);
                if(particles[idx].present==false){particles[idx]=particle;particles[idx].present=true;}
            }
            if (
                asteroidCount < MAX_ASTEROIDS &&
                GetRandomValue(0, 100) < fminf(20,(int)(3+score/10))
            )
            {
                Asteroid asteroid =
                    { 0 };

                asteroid.position =
                    (Vector3)
                    {
                        (float)GetRandomValue(
                            -20,
                            20
                        ),

                        (float)GetRandomValue(
                            -10,
                            10
                        ),

                        35.0f
                    };

                asteroid.speed =
                    5.0f +
                    (float)GetRandomValue(
                        0,
                        30
                    ) / 10.0f;

                asteroid.rotationAxis =
                    Vector3Normalize(
                        (Vector3)
                        {
                            (float)GetRandomValue(
                                -100,
                                100
                            ),

                            (float)GetRandomValue(
                                -100,
                                100
                            ),

                            (float)GetRandomValue(
                                -100,
                                100
                            )
                        }
                    );

                asteroid.rotationSpeed =
                    (float)GetRandomValue(
                        20,
                        100
                    );

                asteroid.rotationAngle =
                    0.0f;

                asteroids[
                    asteroidCount
                ] =
                    asteroid;

                asteroidCount++;
            }

            // ------------------------------------------------
            // Update asteroids
            // ------------------------------------------------

            for (
                int i = 0;
                i < asteroidCount;
                i++
            )
            {
                if(IsKeyDown(KEY_LEFT_SHIFT)){asteroids[i].position.z-=asteroids[i].speed*dt*2.0f+score/1000;particles[i].position.z-=particles[i].speed*dt*8.0f+score/1000;}else{
                
                asteroids[i]
                    .position.z -=
                    asteroids[i].speed *
                    dt+score/1000;
                particles[i].position.z-=particles[i].speed*dt*4.0f+score/1000;
                }
                asteroids[i]
                    .rotationAngle +=
                    asteroids[i].rotationSpeed *
                    dt;
            }

            // ------------------------------------------------
            // Remove asteroids behind camera
            // ------------------------------------------------

            int newCount =
                0;
            
            for (
                int i = 0;
                i < asteroidCount;
                i++
            )
            {
                if (
                    asteroids[i]
                        .position.z > -10.0f
                )
                {
                    asteroids[
                        newCount++
                    ] =
                        asteroids[i];
                }
            }

            asteroidCount =
                newCount;
            for(int i=0;i<100;i++){
                if(particles[i].position.z>-10.0f&&particles[i].present==true){particles[i].present=false;}
            }
            // ------------------------------------------------
            // Collision
            // ------------------------------------------------

            for (
                int i = 0;
                i < asteroidCount;
                i++
            )
            {
                if (
                    CheckCollisionSpheres(
                        shipPos,
                        SHIP_RADIUS,

                        asteroids[i].position,
                        ASTEROID_RADIUS
                    )
                )
                {
                    gameOver =
                        true;

                    break;
                }
            }
        }
        else
        {
            // ------------------------------------------------
            // Restart
            // ------------------------------------------------

            if (
                IsKeyPressed(KEY_R)
            )
            {
                gameOver =
                    false;

                score =
                    0.0f;

                asteroidCount =
                    0;

                shipPos =
                    (Vector3)
                    {
                        0.0f,
                        0.0f,
                        0.0f
                    };
            }
        }

        // ====================================================
        // DRAW
        // ====================================================
        
        BeginDrawing();

        ClearBackground(
            BLACK
        );
        

    
        // ----------------------------------------------------
        // 2D sky
        // ----------------------------------------------------

        if (
            IsTextureValid(
                skyTexture
            )
        )
        {
            DrawTexturePro(
                skyTexture,

                (Rectangle)
                {
                    0.0f,
                    0.0f,

                    (float)skyTexture.width,
                    (float)skyTexture.height
                },

                (Rectangle)
                {
                    0.0f,
                    0.0f,

                    (float)SCREEN_WIDTH,
                    (float)SCREEN_HEIGHT
                },

                (Vector2)
                {
                    0.0f,
                    0.0f
                },

                0.0f,

                WHITE
            );
        }

        // ====================================================
        // 3D
        // ====================================================
        
        
        BeginMode3D(
            camera
        );

        // ----------------------------------------------------
        // ASTEROIDS
        //
        // Just like the raylib PBR example:
        //
        // 1. Set tiling
        // 2. Set emissive color
        // 3. Set metallic
        // 4. Set roughness
        // 5. Draw model
        // ----------------------------------------------------

        SetShaderValue(
            pbrShader,
            textureTilingLoc,
            &asteroidTextureTiling,
            SHADER_UNIFORM_VEC2
        );

        Vector4 asteroidEmissiveColor =
            ColorNormalize(
                asteroidModel.materials[0]
                    .maps[MATERIAL_MAP_EMISSION]
                    .color
            );

        SetShaderValue(
            pbrShader,
            emissiveColorLoc,
            &asteroidEmissiveColor,
            SHADER_UNIFORM_VEC4
        );

        float asteroidEmissiveIntensity =
            0.0f;

        SetShaderValue(
            pbrShader,
            emissiveIntensityLoc,
            &asteroidEmissiveIntensity,
            SHADER_UNIFORM_FLOAT
        );

        float asteroidMetallic =
            asteroidModel.materials[0]
                .maps[MATERIAL_MAP_METALNESS]
                .value;

        float asteroidRoughness =
            asteroidModel.materials[0]
                .maps[MATERIAL_MAP_ROUGHNESS]
                .value;

        SetShaderValue(
            pbrShader,
            metallicValueLoc,
            &asteroidMetallic,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(
            pbrShader,
            roughnessValueLoc,
            &asteroidRoughness,
            SHADER_UNIFORM_FLOAT
        );

        // ----------------------------------------------------
        // Draw asteroids
        // ----------------------------------------------------
        for(int i=0;i<100;i++){
            if(!(particles[i].position.z>30.0f||(particles[i].position.x<=0.1f&&particles[i].position.x>= -0.1f)
            || (particles[i].position.y<=0.1f&&particles[i].position.y>= -0.1f))){
            DrawCylinderEx(particles[i].position,
                (Vector3){particles[i].position.x,particles[i].position.y,particles[i].position.z+particles[i].speed/10},
                0.01f,0.005f,6,(Color){255,255,255,255});
            }
        }
        for (
            int i = 0;
            i < asteroidCount;
            i++
        )
        {
            DrawModelEx(
                asteroidModel,

                asteroids[i].position,

                asteroids[i].rotationAxis,

                asteroids[i].rotationAngle,

                (Vector3)
                {
                    ASTEROID_SCALE,
                    ASTEROID_SCALE,
                    ASTEROID_SCALE
                },

                WHITE
            );
        }

        // ----------------------------------------------------
        // SHIP
        // ----------------------------------------------------

        SetShaderValue(
            pbrShader,
            textureTilingLoc,
            &shipTextureTiling,
            SHADER_UNIFORM_VEC2
        );

        Vector4 shipEmissiveColor =
            ColorNormalize(
                shipModel.materials[0]
                    .maps[MATERIAL_MAP_EMISSION]
                    .color
            );

        SetShaderValue(
            pbrShader,
            emissiveColorLoc,
            &shipEmissiveColor,
            SHADER_UNIFORM_VEC4
        );

        float shipEmissiveIntensity =
            0.0f;

        SetShaderValue(
            pbrShader,
            emissiveIntensityLoc,
            &shipEmissiveIntensity,
            SHADER_UNIFORM_FLOAT
        );

        float shipMetallic =
            shipModel.materials[0]
                .maps[MATERIAL_MAP_METALNESS]
                .value;

        float shipRoughness =
            shipModel.materials[0]
                .maps[MATERIAL_MAP_ROUGHNESS]
                .value;

        SetShaderValue(
            pbrShader,
            metallicValueLoc,
            &shipMetallic,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(
            pbrShader,
            roughnessValueLoc,
            &shipRoughness,
            SHADER_UNIFORM_FLOAT
        );

        // ----------------------------------------------------
        // Draw spaceship
        // ----------------------------------------------------

        /*DrawModel(
            shipModel,
            shipPos,
            SHIP_SCALE,
            WHITE
        );*/

        // ----------------------------------------------------
        // Light visualization
        //
        // This does NOT affect the PBR lighting.
        // It merely lets you see where the light is.
        // ----------------------------------------------------

        Color lightColor =
            (Color)
            {
                (unsigned char)
                    (skyLight.color[0] * 255.0f),

                (unsigned char)
                    (skyLight.color[1] * 255.0f),

                (unsigned char)
                    (skyLight.color[2] * 255.0f),

                255
            };
DrawCylinderEx(
    (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-1.5f},
    (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-1.75f},
    0.2f,0.15f,6,
    (Color){060,040,175,180}
);
DrawCylinderEx(
    (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-1.5f},
    (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-1.75f},
    0.2f,0.15f,6,
    (Color){060,040,175,180}
);
DrawCylinderEx(
    (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-1.75f},
    (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-3.0f},
    0.1f,0.0f,6,
    (Color){150,000,255,180}
);
DrawCylinderEx(
    (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-1.75f},
    (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-3.0f},
    0.1f,0.0f,6,
    (Color){150,000,255,180}
);
if(accelX>0.0001f){DrawCylinderEx(
        (Vector3){shipPos.x-2.0f,shipPos.y,shipPos.z-0.5f},
        (Vector3){shipPos.x-3.0f-accelX*300.0f,shipPos.y,shipPos.z-1.0f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-2.0f,shipPos.y,shipPos.z-0.5f},
        (Vector3){shipPos.x-2.25f-accelX*50.0f,shipPos.y,shipPos.z-1.0f},
        0.1f,0.2f,4,
        (Color){255,120,200,220}
    );}
else if(accelX<-0.0001f){DrawCylinderEx(
        (Vector3){shipPos.x+2.0f,shipPos.y,shipPos.z-0.5f},
        (Vector3){shipPos.x+3.0f-accelX*300.0f,shipPos.y,shipPos.z-1.0f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x+2.0f,shipPos.y,shipPos.z-0.5f},
        (Vector3){shipPos.x+2.25f-accelX*50.0f,shipPos.y,shipPos.z-1.0f},
        0.1f,0.2f,4,
        (Color){255,120,200,220}
    );}
else{}
if(accelY>0.0001f){DrawCylinderEx(
        (Vector3){shipPos.x+0.5f,shipPos.y-0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x+1.0f,shipPos.y-1.5f-accelY*200.0f,shipPos.z-0.5f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.5f,shipPos.y-0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x-1.0f,shipPos.y-1.5f-accelY*200.0f,shipPos.z-0.5f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.25f,shipPos.y-0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x-0.5f,shipPos.y-0.75f-accelY*50.0f,shipPos.z-0.5f},
        0.2f,0.0f,4,
        (Color){255,120,200,220}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x+0.25f,shipPos.y-0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x+0.5f,shipPos.y-0.75f-accelY*50.0f,shipPos.z-0.5f},
        0.2f,0.0f,4,
        (Color){255,120,200,220}
    );}
else if(accelY<-0.0001f){DrawCylinderEx(
        (Vector3){shipPos.x+0.5f,shipPos.y+0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x+1.0f,shipPos.y+1.5f-accelY*200.0f,shipPos.z-1.0f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.5f,shipPos.y+0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x-1.0f,shipPos.y+1.5f-accelY*200.0f,shipPos.z-1.0f},
        0.1f,0.0f,6,
        (Color){255,100,030,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x+0.25f,shipPos.y+0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x+0.5f,shipPos.y+0.75f-accelY*50.0f,shipPos.z-0.5f},
        0.2f,0.0f,6,
        (Color){255,120,200,220}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.25f,shipPos.y+0.25f,shipPos.z-0.5f},
        (Vector3){shipPos.x-0.5f,shipPos.y+0.75f-accelY*50.0f,shipPos.z-0.5f},
        0.2f,0.0f,6,
        (Color){255,129,200,220}
    );}
else{}
DrawModelEx(shipModel,shipPos,(Vector3){0.0f,0.0f,1.0f},0.0f,(Vector3){SHIP_SCALE,SHIP_SCALE,SHIP_SCALE},(Color){255,255,255,255});
if(IsKeyDown(KEY_LEFT_SHIFT)){
    DrawCylinderEx(
        (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-1.5f},
        (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-3.0f},
        0.3f,0.1f,6,
        (Color){255,050,255,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-1.5f},
        (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-3.0f},
        0.3f,0.1f,6,
        (Color){255,050,255,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-3.0f},
        (Vector3){shipPos.x+0.5f,shipPos.y,shipPos.z-4.0f},
        0.1f,0.0f,6,
        (Color){150,050,255,180}
    );
    DrawCylinderEx(
        (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-3.0f},
        (Vector3){shipPos.x-0.5f,shipPos.y,shipPos.z-4.0f},
        0.1f,0.0f,6,
        (Color){150,050,255,180}
    );
}

        EndMode3D();
        // ====================================================
        // UI
        // ====================================================

        DrawText(
            TextFormat(
                "Score: %d",
                (int)score
            ),
            20,
            20,
            24,
            WHITE
        );

        DrawText(
            TextFormat(
                "Asteroids: %d",
                asteroidCount
            ),
            20,
            50,
            20,
            YELLOW
        );

        DrawText(
            "WASD / Arrow Keys / TouchScreen - Move | LShift - Boost",
            20,
            SCREEN_HEIGHT - 35,
            20,
            WHITE
        );

        if (gameOver)
        {
            DrawText(
                "GAME OVER!",
                SCREEN_WIDTH / 2 - 120,
                280,
                40,
                RED
            );

            DrawText(
                "Press R to restart",
                SCREEN_WIDTH / 2 - 110,
                330,
                25,
                WHITE
            );
        }
        EndDrawing();
        
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    UnloadModel(
        shipModel
    );

    UnloadModel(
        asteroidModel
    );

    UnloadTexture(
        skyTexture
    );

    UnloadTexture(
        dummyAlbedo
    );

    UnloadTexture(
        dummyMRA
    );

    UnloadTexture(
        dummyNormal
    );

    UnloadTexture(
        dummyEmissive
    );

    UnloadShader(
        pbrShader
    );
    UnloadShader(bloomShader);
    CloseWindow();

    return 0;
}

//emcc -o index.html main.c -Llib/ -Iinclude/ ./lib/libraylib.web.a -s USE_GLFW=3 -s ASYNCIFY --embed-file resources/ -DPLATFORM_WEB -s ALLOW_MEMORY_GROWTH