#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_RICONS
#include "../lib/raygui/raygui.h"
#undef RAYGUI_IMPLEMENTATION

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
} Game;

//----------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------

static Game game = {
        .window = { 800,450,"raylib template - simple game" },
        .ui = {0},
        .textures = {0},
        .currentScreen = LOGO
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

    // workaround for mac high dpi; screen gets rendered in the bottom left quadrant of the window
    // until the window moves, so just shimmy it a bit to get things rendering correctly right away
    {
        Vector2 windowPos = GetWindowPosition();
        SetWindowPosition((int) windowPos.x + 1, (int) windowPos.y);
        SetWindowPosition((int) windowPos.x, (int) windowPos.y);
    }

    const int buttonWidth = 250;
    const int buttonHeight = 40;
    game.ui.buttonPos_StartGame = (Rectangle) {game.window.width / 2 - buttonWidth / 2, game.window.height / 2 - buttonHeight / 2, buttonWidth, buttonHeight };
    game.ui.buttonText_StartGame = "Click to Start";
    game.ui.font = LoadFont("../assets/fonts/open-sans-regular.ttf");
    GuiSetFont(game.ui.font);

    game.textures.test = LoadTexture("../assets/test.png");

    SetTargetFPS(60);
}

// ------------------------------------------------------------------

void Update() {
    static int framesCounter = 0;

    switch(game.currentScreen)
    {
        case LOGO:
        {
            // TODO: Update LOGO screen variables here!

            framesCounter++;    // Count frames

            // Wait for 2 seconds (120 frames) before jumping to TITLE screen
            if (framesCounter > 120)
            {
                game.currentScreen = TITLE;
            }
        } break;
        case TITLE:
        {
            // TODO: Update TITLE screen variables here!

        } break;
        case GAMEPLAY:
        {
            // TODO: Update GAMEPLAY screen variables here!

            // Press enter to change to ENDING screen
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
            {
                game.currentScreen = ENDING;
            }
        } break;
        case ENDING:
        {
            // TODO: Update ENDING screen variables here!

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
            // TODO: Draw LOGO screen here!
            DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
            DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);

        } break;
        case TITLE:
        {
            // TODO: Draw TITLE screen here!
            DrawRectangle(0, 0, game.window.width, game.window.height, GREEN);

            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);
            if (GuiButton(game.ui.buttonPos_StartGame, GuiIconText(RICON_DOOR, game.ui.buttonText_StartGame))) {
                game.currentScreen = GAMEPLAY;
            }

            DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
        } break;
        case GAMEPLAY:
        {
            // TODO: Draw GAMEPLAY screen here!
            DrawRectangle(0, 0, game.window.width, game.window.height, PURPLE);

            DrawTexture(game.textures.test, game.window.width / 2 - game.textures.test.width / 2, game.window.height / 2 - game.textures.test.height / 2, WHITE);

            DrawText("GAMEPLAY SCREEN", 20, 20, 40, MAROON);
            DrawText("PRESS ENTER or TAP to JUMP to ENDING SCREEN", 130, 220, 20, MAROON);

        } break;
        case ENDING:
        {
            // TODO: Draw ENDING screen here!
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
}
