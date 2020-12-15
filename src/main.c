#include "raylib.h"
#include "raymath.h"

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

typedef enum GameScreen { LOGO = 0, TITLE, GAMEPLAY, EDITING } GameScreen;

typedef struct Window {
    int width;
    int height;
    const char *title;
} Window;

typedef struct UserInterface {
    Font font;
    bool show;

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

static Game game = {0};

char textBuffer[1000] = {0};
char gamepadName[1000] = {0};

//----------------------------------------------------------------------------------
// Lifecycle function declarations
//----------------------------------------------------------------------------------

void Initialize();
void Update();
void Draw();
void Unload();

void updateCamera(Vector2 vector2, float dt);

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
    game.currentScreen = LOGO;

    game.window = (Window) { 800, 450, "raylib template - simple game" };
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
    game.ui.show = false;
    GuiSetFont(game.ui.font);
    GuiLoadStyle("../assets/ui/cyber.rgs");

    game.textures.test = LoadTexture("../assets/test.png");
    loadAssets();

    initializeWorld(&game.world);

    game.player.bounds = (Rectangle) {200, -32, 32, 32 };
    game.player.center = getCenter(game.player.bounds);
    game.player.animation = getAnimation(character_idle_right);
    game.player.facing = right;

    game.camera.target = game.player.center;
    game.camera.offset = (Vector2) { game.window.width / 2, game.window.height / 2 };
    game.camera.rotation = 0;
    game.camera.zoom = 1;
}

// ------------------------------------------------------------------

void Update() {
    static int framesCounter = 0;

    const float dt = GetFrameTime();
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
            if (IsGamepadAvailable(GAMEPAD_PLAYER1)) {
                sprintf(gamepadName, "%s", GetGamepadName(GAMEPAD_PLAYER1));
            }

            updatePlayer(&game.player, &game.world, dt);
            updateCamera(game.player.center, dt);

            // Press enter to change to EDITING screen
            if (IsKeyPressed(KEY_ENTER)) // || IsGestureDetected(GESTURE_TAP))
            {
                game.currentScreen = EDITING;
            }
        } break;
        case EDITING:
        {
            // Press enter to return to TITLE screen
            if (IsKeyPressed(KEY_ENTER)) // || IsGestureDetected(GESTURE_TAP))
            {
                game.currentScreen = TITLE;
            }
        } break;
        default: break;
    }
}

void updateCamera(Vector2 target, float dt) {
    static float minSpeed = 50;
    static float minEffectLength = 20;
    static float fractionSpeed = 0.9f;

    game.camera.offset = (Vector2) { game.window.width / 2, game.window.height / 2 };
    Vector2 diff = Vector2Subtract(target, game.camera.target);
    float length = Vector2Length(diff);

    if (length > minEffectLength) {
        float speed = fmaxf(fractionSpeed * length, minSpeed);
        float scale = speed * dt / length;
        Vector2 deltaTarget = Vector2Scale(diff, scale);
        game.camera.target = Vector2Add(game.camera.target, deltaTarget);
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
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
            DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);
        } break;
        case TITLE:
        {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            DrawRectangle(0, 0, game.window.width, game.window.height, GREEN);

            GuiSetStyle(BUTTON, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);
            if (GuiButton(game.ui.buttonPos_StartGame, GuiIconText(RICON_DOOR, game.ui.buttonText_StartGame))) {
                game.currentScreen = GAMEPLAY;
            }
            DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
        } break;
        case GAMEPLAY:
        {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

            DrawRectangle(0, 0, game.window.width, game.window.height, PURPLE);

            BeginMode2D(game.camera);
            {
                for (int i = 0; i < stb_arr_len(game.world.solids); ++i) {
                    Solid solid = game.world.solids[i];
                    DrawRectangle(solid.bounds.x, solid.bounds.y, solid.bounds.width, solid.bounds.height, BROWN);
                }

                int hFlip = (game.player.facing == left) ? -1 : 1;
                Texture2D texture = getAnimationFrame(&game.player.animation, game.player.stateTime);
                Rectangle srcRect = { 0, 0, hFlip * texture.width, texture.height };
                Rectangle dstRect = game.player.bounds;
                Vector2 origin = {0};
                float rotation = 0;
                DrawTexturePro(texture, srcRect, dstRect, origin, rotation, WHITE);
            }
            EndMode2D();

            if (game.ui.show) {
                const float margin = 10;
                Rectangle panel = {
                        margin,
                        margin,
                        game.window.width - 2 * margin,
                        100
                };
                if (GuiWindowBox(panel, "Gameplay Screen")) {
                    printf("Clicked window box\n");
                    game.ui.show = false;
                }

                if (IsGamepadAvailable(GAMEPAD_PLAYER1)) {
                    Rectangle gamepadBoxBounds = {
                            panel.x + margin,
                            panel.y + WINDOW_STATUSBAR_HEIGHT + margin,
                            panel.width - 2 * margin,
                            panel.height - WINDOW_STATUSBAR_HEIGHT - 2 * margin
                    };
                    GuiGroupBox(gamepadBoxBounds, "Gamepad");
                    GuiDrawText(gamepadName, gamepadBoxBounds, GUI_TEXT_ALIGN_CENTER, WHITE);

                    Texture2D icon = assets.icon_gamepad_ps;
                    float scaledHeight = gamepadBoxBounds.height - 2 * margin;
                    float scaledWidth = icon.width * (scaledHeight / icon.height);
                    Rectangle srcRect = { 0, 0, icon.width, icon.height };
                    Rectangle dstRect = {
                            gamepadBoxBounds.x + margin,
                            gamepadBoxBounds.y + gamepadBoxBounds.height / 2 - scaledHeight / 2,
                            scaledWidth, scaledHeight
                    };
                    Vector2 origin = {0};
                    float rotation = 0;
                    DrawTexturePro(icon, srcRect, dstRect, origin, rotation, GREEN);
                }
            } else {
                float margin = 10;
                Rectangle panel = { margin, margin, 120, 50 };
                GuiPanel(panel);

                Texture2D icon = assets.icon_gamepad_xbox;
                float scaledHeight = panel.height - 2 * margin;
                float scaledWidth = icon.width * (scaledHeight / icon.height);
                Rectangle srcRect = { 0, 0, icon.width, icon.height };
                Rectangle dstRect = {
                        panel.x,
                        panel.y + panel.height / 2 - scaledHeight / 2,
                        scaledWidth, scaledHeight
                };
                Vector2 origin = {0};
                float rotation = 0;
                DrawTexturePro(icon, srcRect, dstRect, origin, rotation, WHITE);

                float buttonHeight = panel.height - 2 * margin;
                Rectangle button = {
                        dstRect.x + dstRect.width,
                        panel.y + panel.height / 2 - buttonHeight / 2,
                        panel.width - scaledWidth,
                        buttonHeight
                };
                if (GuiLabelButton(button, "Menu")) {
                    game.ui.show = true;
                }
            }
        } break;
        case EDITING:
        {
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);

            DrawRectangle(0, 0, game.window.width, game.window.height, BLUE);
            DrawText("EDITING SCREEN", 20, 20, 40, DARKBLUE);

            BeginMode2D(game.camera);
            {
                for (int i = 0; i < stb_arr_len(game.world.solids); ++i) {
                    Solid solid = game.world.solids[i];
                    DrawRectangle(solid.bounds.x, solid.bounds.y, solid.bounds.width, solid.bounds.height, (Color) { 0, 250, 250, 128 });
                }

                static bool mouseDragging    = false;
                static bool mousePressedLeft = false;
                static bool mouseDraggedLeft = false;
                static Vector2 draggedStart = {0};

                Vector2 mousePos = GetMousePosition();
                Vector2 worldPos = GetScreenToWorld2D(mousePos, game.camera);

                if (mouseDraggedLeft) {
                    Vector2 draggedEnd = worldPos;
                    Rectangle bounds = { draggedStart.x, draggedStart.y, draggedEnd.x - draggedStart.x, draggedEnd.y - draggedStart.y };
                    DrawRectangleGradientV(bounds.x, bounds.y, bounds.width, bounds.height, LIME, GREEN);
                }

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    if (!mousePressedLeft) {
                        mousePressedLeft = true;
                        draggedStart = worldPos;
                    } else {
                        mouseDraggedLeft = true;
                    }
                } else if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
                    if (mouseDraggedLeft) {
                        Vector2 draggedEnd = worldPos;
                        // TODO: check for and fixup negative rectangle sizes (if draggedEnd < draggedStart on any axis)
                        Rectangle bounds = { draggedStart.x, draggedStart.y, draggedEnd.x - draggedStart.x, draggedEnd.y - draggedStart.y };
                        stb_arr_push(game.world.solids, (Solid) { bounds });
                        TraceLog(LOG_INFO, "Added solid (%.1f, %.1f, %.0f, %.0f)", bounds.x, bounds.y, bounds.width, bounds.height);
                    } else {
                        // TODO: handle selecting an existing solid
                    }

                    mousePressedLeft = false;
                    mouseDraggedLeft = false;
                }
            }
            EndMode2D();

            DrawText("PRESS ENTER to RETURN to TITLE SCREEN", 120, game.window.height - 30, 20, DARKBLUE);
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

    unloadAssets();
}
