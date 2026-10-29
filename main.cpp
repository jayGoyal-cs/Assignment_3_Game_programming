#include "CS3113/Entity.h"
#include <cmath>

/**
* Author: Jay Goyal
* Assignment: Lunar Lander
* Date due: 10/29/2026
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// Global Constants
constexpr int SCREEN_WIDTH  = 1000,
              SCREEN_HEIGHT = 600,
              FPS           = 120;

constexpr char    BG_COLOUR[] = "#000000";
constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

constexpr int   NUMBER_OF_PAD_TILES = 4,
                NUMBER_OF_TILES = 7;
constexpr float TILE_DIMENSION      = 50.0f,
                ACCELERATION_OF_GRAVITY = 100.0f,  // moon gravity
                VERTICAL_THRUST     = 260.0f,      // bigger than gravity so we can climb
                HORIZONTAL_THRUST   = 200.0f,
                DRAG                = 1.2f,        // slows us down when no key is held
                MAX_FUEL            = 100.0f,
                SAFE_LANDING_SPEED  = 120.0f,      // crash speed
                SPEED_OF_BEAM       = 500.0f;     // Speed of beam
// Global Variables
AppStatus gAppStatus     = RUNNING;
float     gPreviousTicks = 0.0f;
float     gTime          = 0.0f;       // used to slide the landing pad
float     gFuel          = MAX_FUEL;
float     gTimeOfFire    = 3.0f;
bool gMissionAccomplished = false;
bool gMissionFailed = false;

Entity *gTaxi     = nullptr;  //Player
Entity *gPadTiles = nullptr;   // moving landing pad
Entity *gTiles = nullptr;     // Obstacles 
Entity *gBeam = nullptr;     // Beam of destruction

Music gBGM;

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

// Checks for collision between two entities
bool CheckEntityCollision(const Entity *a, const Entity *b)
{
    float xDistance = std::abs(a->getPosition().x - b->getPosition().x) -
        ((a->getColliderDimensions().x + b->getColliderDimensions().x) / 2.0f);
    float yDistance = std::abs(a->getPosition().y - b->getPosition().y) -
        ((a->getColliderDimensions().y + b->getColliderDimensions().y) / 2.0f);

    return xDistance < 0.0f && yDistance < 0.0f;
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lunar Lander");
    InitAudioDevice();
    /*
        ----------- TAXI -----------
    */
    std::map<Direction, std::vector<int>> animationAtlas = {
        { STOP, { 0 } },
        { FLY,  { 1, 2, 3 } }
    };

    gTaxi = new Entity(
        { ORIGIN.x, 80.0f },    // position
        { 64.0f, 64.0f },       // scale
        "assets/game/flyingTaxi.png",       // texture file address
        ATLAS,                  // single image or atlas?
        { 2, 2 },               // atlas dimensions
        animationAtlas,         // actual atlas
        PLAYER
    );

    gTaxi->setColliderDimensions({ 48.0f, 44.0f });
    gTaxi->setAcceleration({ 0.0f, ACCELERATION_OF_GRAVITY });
    gTaxi->setDirection(STOP);

    /*
        ----------- LANDING PAD/TILES -----------
        (positions are set every frame in update())
    */
    gPadTiles = new Entity[NUMBER_OF_PAD_TILES];
    gTiles    = new Entity[NUMBER_OF_TILES];
    for (int i = 0; i < NUMBER_OF_PAD_TILES; i++)
    {
        gPadTiles[i].setTexture("assets/game/tile_0061.png");
        gPadTiles[i].setScale({ TILE_DIMENSION, TILE_DIMENSION });
        gPadTiles[i].setColliderDimensions({ TILE_DIMENSION, TILE_DIMENSION });
    }

    for (int i = 0; i < NUMBER_OF_TILES; i++)
    {
        gTiles[i].setTexture("assets/game/tile_0000.png");
        gTiles[i].setScale({ TILE_DIMENSION, TILE_DIMENSION });
        gTiles[i].setColliderDimensions({ TILE_DIMENSION, TILE_DIMENSION });
    }

    
    // Start clock now because then everything starts 
    // and kills the player instantly 
    gPreviousTicks = static_cast<float>(GetTime());

    //bgm music
    gBGM = LoadMusicStream("assets/game/Le_Grand_Chase.mp3");
    SetMusicVolume(gBGM, 0.5f);
    PlayMusicStream(gBGM);

    SetTargetFPS(FPS);
}

void processInput()
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;

    // Stop taking input once the game is won or loss
    if (gMissionAccomplished) return;
    if (gMissionFailed)       return;

    // Gravity always pulls down drag pushes against our sideways speed
    // so the ship drifts and slowly stops when we let go of the keys
    Vector2   acceleration = { -DRAG * gTaxi->getVelocity().x, ACCELERATION_OF_GRAVITY };
    Direction direction    = STOP;

    // Thrusters only work while we still have fuel
    if (gFuel > 0.0f)
    {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            acceleration.y -= VERTICAL_THRUST;
            gFuel -= 15.0f * GetFrameTime();
            direction = FLY;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            acceleration.x -= HORIZONTAL_THRUST;
            gFuel -= 8.0f * GetFrameTime();
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            acceleration.x += HORIZONTAL_THRUST;
            gFuel -= 8.0f * GetFrameTime();
        }
    }

    if (gFuel < 0.0f) gFuel = 0.0f;

    //Change acceleration so that the taxi can move
    gTaxi->setAcceleration(acceleration);

    // switch animation
    if (gTaxi->getDirection() != direction) gTaxi->setDirection(direction);
}
void update()
{
    
    UpdateMusicStream(gBGM);
    float ticks     = static_cast<float>(GetTime());
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    if (gMissionAccomplished) return;
    if (gMissionFailed) return;
    gTime += deltaTime;
    
    // Slide landing pad & tiles
    float padX   = ORIGIN.x + 200.0f * std::sin(gTime * 0.8f);
    float tilesX = ORIGIN.x + 200.0f * std::cos(gTime * 0.8f);

    for (int i = 0; i < NUMBER_OF_PAD_TILES; i++){
        gPadTiles[i].setPosition({
            padX + (i - (NUMBER_OF_PAD_TILES - 1) / 2.0f) * TILE_DIMENSION,
            500.0f
        });
    }

    for (int i = 0; i < NUMBER_OF_TILES; i++){
        gTiles[i].setPosition({
            tilesX + (i - (NUMBER_OF_TILES - 1) / 2.0f) * TILE_DIMENSION,
            250.0f
        });
    }

    gTaxi->update(deltaTime, nullptr, 0);
    for (int i = 0; i < NUMBER_OF_TILES; i++){   
        gTiles[i].update(deltaTime, gTaxi, 1);
        if (CheckEntityCollision(gTaxi, &gTiles[i])) {
            gMissionFailed = true;
        }
    }

    // Screen bound checks for player
    Vector2 pos = gTaxi->getPosition();
    if (pos.x < 0 || pos.x > SCREEN_WIDTH || pos.y < 0 || pos.y > SCREEN_HEIGHT){
        gMissionFailed = true;
    }

    // Landing pad collision check
    for (int i = 0; i < NUMBER_OF_PAD_TILES; i++){
        if (CheckEntityCollision(gTaxi, &gPadTiles[i]))
        {
            if (gTaxi->getVelocity().y < SAFE_LANDING_SPEED) gMissionAccomplished = true;
            else                                              gAppStatus = TERMINATED;
            break;
        }
    }

 
   if (gBeam == nullptr){
    Vector2 spawnPos = { 0.0f, 0.0f };

    gBeam = new Entity(
        spawnPos,
        { TILE_DIMENSION / 2.0f, TILE_DIMENSION / 2.0f },
        "assets/game/tile_0000.png",
        BLOCK 
    );
    //BEAM angle and direction
    Vector2 diff = { gTaxi->getPosition().x - spawnPos.x, gTaxi->getPosition().y - spawnPos.y };
    Normalise(&diff);

    gBeam->setVelocity({ diff.x * SPEED_OF_BEAM, diff.y * SPEED_OF_BEAM });
}

    // Update beam movement & check bounds / hit
    if (gBeam != nullptr){
        gBeam->update(deltaTime, nullptr, 0);

        if (CheckEntityCollision(gTaxi, gBeam)){
           gMissionFailed = true; // Player hit by beam
        }

        Vector2 bPos = gBeam->getPosition();
        if (bPos.x < 0 || bPos.x > SCREEN_WIDTH || bPos.y < 0 || bPos.y > SCREEN_HEIGHT){
            delete gBeam;
            gBeam = nullptr;
        }
    }
    
}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    for (int i = 0; i < NUMBER_OF_PAD_TILES; i++) gPadTiles[i].render();
    for (int i = 0; i < NUMBER_OF_TILES; i++) gTiles[i].render();

    gTaxi->render();

    if (gBeam != nullptr) gBeam->render();

    DrawText(TextFormat("FUEL: %d", static_cast<int>(gFuel)), 20, 15, 30, WHITE);

    if (gMissionAccomplished) DrawText("Mission Accomplished", 230, 250, 50, GREEN);

    if(gMissionFailed) DrawText("Mission Failed", 230, 250, 50, RED);
    EndDrawing();
}
void shutdown()
{
    delete gTaxi;
    delete[] gPadTiles;
    delete[] gTiles;
    UnloadMusicStream(gBGM);
    if (gBeam != nullptr) delete gBeam;
    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}