#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_RICONS
#include "../lib/raygui/raygui.h"
#undef RAYGUI_IMPLEMENTATION

#define STB_DEFINE
#include "../lib/stb/stb.h"

#include "world.h"

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef enum GameScreen { LOGO = 0, TITLE, GAMEPLAY, ENDING } GameScreen;

typedef struct Window {
    int width;
    int height;
    const char *title;
} Window;

typedef struct UserInterface {
    Font font;
    Rectangle buttonPos_StartGame;
    const char *buttonText_StartGame;
} UserInterface;

typedef struct Textures {
    Texture test;
} Textures;

typedef struct Game {
    Window window;
    UserInterface ui;
    Textures textures;
    GameScreen currentScreen;
    Camera2D camera;
    World world;
    Actor player;
} Game;

//----------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------

static Game game = {
        .window = { 800,450,"raylib template - simple game" },
        .ui = {0},
        .textures = {0},
        .currentScreen = LOGO,
        .camera = {0},
        .world = {0},
        .player = {0}
};

//----------------------------------------------------------------------------------
// Lifecycle function declarations
//----------------------------------------------------------------------------------

void Initialize();
void Update();
void Draw();
void Unload();

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    Initialize();
    while (!WindowShouldClose()) {
        Update();
        Draw();
    }
    Unload();
    CloseWindow();
    return 0;
}

//----------------------------------------------------------------------------------
// Lifecycle function implementation
//----------------------------------------------------------------------------------

void Initialize() {
    InitWindow(game.window.width, game.window.height, game.window.title);
    SetTargetFPS(60);

    // workaround for mac high dpi; screen gets rendered in the bottom left quadrant of the window
    // until the window moves, so just shimmy it a bit to get things rendering correctly right away
    {
        Vector2 windowPos = GetWindowPosition();
        SetWindowPosition((int) windowPos.x + 1, (int) windowPos.y);
        SetWindowPosition((int) windowPos.x, (int) windowPos.y);
    }

    // --------------------------------

    const int buttonWidth = 250;
    const int buttonHeight = 40;
    game.ui.buttonPos_StartGame = (Rectangle) {game.window.width / 2 - buttonWidth / 2, game.window.height / 2 - buttonHeight / 2, buttonWidth, buttonHeight };
    game.ui.buttonText_StartGame = "Click to Start";
    game.ui.font = LoadFont("../assets/fonts/open-sans-regular.ttf");
    GuiSetFont(game.ui.font);

    game.textures.test = LoadTexture("../assets/test.png");

    initializeWorld(&game.world);

    game.player.bounds = (Rectangle) {225, -100, 50, 100 };
    game.player.center = getCenter(game.player.bounds);

    game.camera.target = game.player.center;
    game.camera.offset = (Vector2) { game.window.width / 2, game.window.height / 2 };
    game.camera.rotation = 0;
    game.camera.zoom = 1;
}

// ------------------------------------------------------------------

void Update() {
    static int framesCounter = 0;

    switch(game.currentScreen)
    {
        case LOGO:
        {
            framesCounter++;

            // Wait for 2 seconds (120 frames) before jumping to TITLE screen
            if (framesCounter > 120)
            {
                game.currentScreen = TITLE;
            }
        } break;
        case TITLE:
        {
            // ...
        } break;
        case GAMEPLAY:
        {
            const float speed = 200;
            const float dt = GetFrameTime();

            if (IsKeyDown(KEY_A)) moveX(&game.player, -speed * dt, &game.world, NULL);
            if (IsKeyDown(KEY_D)) moveX(&game.player,  speed * dt, &game.world, NULL);
            if (IsKeyDown(KEY_W)) moveY(&game.player, -speed * dt, &game.world, NULL);
            if (IsKeyDown(KEY_S)) moveY(&game.player,  speed * dt, &game.world, NULL);

            game.camera.target = game.player.center;

            // Press enter to change to ENDING screen
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
            {
                game.currentScreen = ENDING;
            }
        } break;
        case ENDING:
        {
            // Press enter to return to TITLE screen
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
            {
                game.currentScreen = TITLE;
            }
        } break;
        default: break;
    }
}

// ------------------------------------------------------------------

void Draw() {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    switch(game.currentScreen)
    {
        case LOGO:
        {
            DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
            DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);
        } break;
        case TITLE:
        {
            DrawRectangle(0, 0, game.window.width, game.window.height, GREEN);

            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);
            if (GuiButton(game.ui.buttonPos_StartGame, GuiIconText(RICON_DOOR, game.ui.buttonText_StartGame))) {
                game.currentScreen = GAMEPLAY;
            }
            DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
        } break;
        case GAMEPLAY:
        {
            DrawRectangle(0, 0, game.window.width, game.window.height, PURPLE);

            BeginMode2D(game.camera);
            {
                for (int i = 0; i < stb_arr_len(game.world.solids); ++i) {
                    Solid solid = game.world.solids[i];
                    DrawRectangle(solid.bounds.x, solid.bounds.y, solid.bounds.width, solid.bounds.height, BROWN);
                }

                Texture texture = game.textures.test;
                Rectangle srcRect = { 0, 0, texture.width, texture.height };
                Rectangle dstRect = game.player.bounds;
                Vector2 origin = {0};
                float rotation = 0;
                DrawTexturePro(texture, srcRect, dstRect, origin, rotation, WHITE);
            }
            EndMode2D();

            DrawText("GAMEPLAY SCREEN", 20, 20, 40, MAROON);
        } break;
        case ENDING:
        {
            DrawRectangle(0, 0, game.window.width, game.window.height, BLUE);
            DrawText("ENDING SCREEN", 20, 20, 40, DARKBLUE);
            DrawText("PRESS ENTER or TAP to RETURN to TITLE SCREEN", 120, 220, 20, DARKBLUE);
        } break;
        default: break;
    }

    EndDrawing();
}

// ------------------------------------------------------------------

void Unload() {
    UnloadTexture(game.textures.test);
    UnloadFont(game.ui.font);

    unloadWorld(&game.world);
}
