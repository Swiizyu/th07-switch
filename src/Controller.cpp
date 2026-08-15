#include "Controller.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

#include "Supervisor.hpp"
#include "Touch.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

static u16 g_AutoFocusTimer;

#define KEY_PRESSED(scancode, thButton) (keys[scancode] ? thButton : 0)
#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)

static const SDL_GamepadButton g_DIToSDLButton[] = {
    SDL_GAMEPAD_BUTTON_SOUTH,         SDL_GAMEPAD_BUTTON_EAST,
    SDL_GAMEPAD_BUTTON_WEST,          SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_BACK,          SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_LEFT_STICK,    SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_GUIDE,
};

u32 Controller::SetButton(u16 *outButtons, i32 controllerButton, u32 thButton)
{
    if (controllerButton < 0 || (size_t)controllerButton >= ARRAY_SIZE(g_DIToSDLButton))
    {
        return 0;
    }

    if (SDL_GetGamepadButton(g_Supervisor.controller, g_DIToSDLButton[controllerButton]))
    {
        *outButtons |= thButton;
        return thButton;
    }

    return 0;
}

u16 Controller::GetControllerInput(u16 buttons)
{
    if (!g_Supervisor.controller)
    {
        return buttons;
    }

#ifdef __SWITCH__
    // th07-switch: fixed layout, identical to the th06 Switch port.
    //   A (SDL east)  = shoot/confirm    B (SDL south) = bomb/cancel
    //   L             = focus            +             = menu/pause
    //   R             = skip dialogue
    // The config mapping is ignored on purpose: th07.cfg cannot express the
    // triggers at all, and letting every face button do something (the stock
    // PC mapping also binds skip and the TH_BUTTON_D cheat key) is exactly the
    // "all the buttons do random things" behaviour we do not want here.
    SDL_Gamepad *pad = g_Supervisor.controller;
    u32 isShooting = 0;

    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST))
    {
        buttons |= TH_BUTTON_SHOOT;
        isShooting = TH_BUTTON_SHOOT;
    }
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH))
    {
        buttons |= TH_BUTTON_BOMB;
    }
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
    {
        buttons |= TH_BUTTON_FOCUS;
    }
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START))
    {
        buttons |= TH_BUTTON_MENU;
    }
    // R is the dialogue skip (Ctrl on PC). Held, not tapped, as in the original.
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
    {
        buttons |= TH_BUTTON_SKIP;
    }
#else
    u32 isShooting =
        SetButton(&buttons, g_Supervisor.cfg.controllerMapping.shootButton, TH_BUTTON_SHOOT);
#endif

    // ZUN's "shot slow" auto-focus: holding shot also engages focus after ~10
    // frames. Off by default on Switch (see Supervisor::LoadConfig), but the
    // options screen can turn it back on and it is honoured when it does.
    if (g_Supervisor.cfg.shotSlow)
    {
        if (isShooting)
        {
            if (g_AutoFocusTimer < 20)
            {
                g_AutoFocusTimer++;
            }
            if (g_AutoFocusTimer >= 10)
            {
                buttons |= TH_BUTTON_FOCUS;
            }
        }
        else if (g_AutoFocusTimer > 10)
        {
            g_AutoFocusTimer -= 10;
            buttons |= TH_BUTTON_FOCUS;
        }
        else
        {
            g_AutoFocusTimer = 0;
        }
    }

#ifndef __SWITCH__
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.bombButton, TH_BUTTON_BOMB);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.focusButton, TH_BUTTON_FOCUS);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.menuButton, TH_BUTTON_MENU);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.upButton, TH_BUTTON_UP);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.downButton, TH_BUTTON_DOWN);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.leftButton, TH_BUTTON_LEFT);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.rightButton, TH_BUTTON_RIGHT);
    SetButton(&buttons, g_Supervisor.cfg.controllerMapping.skipButton, TH_BUTTON_SKIP);

    SetButton(&buttons, 7, TH_BUTTON_D);
#endif

    Sint16 x = SDL_GetGamepadAxis(g_Supervisor.controller, SDL_GAMEPAD_AXIS_LEFTX) / 32.767f;
    Sint16 y = SDL_GetGamepadAxis(g_Supervisor.controller, SDL_GAMEPAD_AXIS_LEFTY) / 32.767f;

    if (x > g_Supervisor.cfg.padAxisX)
    {
        buttons |= TH_BUTTON_RIGHT;
    }

    if (x < -g_Supervisor.cfg.padAxisX)
    {
        buttons |= TH_BUTTON_LEFT;
    }

    if (y > g_Supervisor.cfg.padAxisY)
    {
        buttons |= TH_BUTTON_DOWN;
    }

    if (y < -g_Supervisor.cfg.padAxisY)
    {
        buttons |= TH_BUTTON_UP;
    }

    // technically the original game never had dpad support but ehhhh
    if (SDL_GetGamepadButton(g_Supervisor.controller, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
    {
        buttons |= TH_BUTTON_RIGHT;
    }
    if (SDL_GetGamepadButton(g_Supervisor.controller, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
    {
        buttons |= TH_BUTTON_LEFT;
    }
    if (SDL_GetGamepadButton(g_Supervisor.controller, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
    {
        buttons |= TH_BUTTON_DOWN;
    }
    if (SDL_GetGamepadButton(g_Supervisor.controller, SDL_GAMEPAD_BUTTON_DPAD_UP))
    {
        buttons |= TH_BUTTON_UP;
    }

    return buttons;
}

static u8 g_ControllerData[32 * 4];

u8 *Controller::GetControllerState()
{
    memset(g_ControllerData, 0, sizeof(g_ControllerData));

    if (!g_Supervisor.controller)
    {
        return g_ControllerData;
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_DIToSDLButton); ++i)
    {
        if (SDL_GetGamepadButton(g_Supervisor.controller, g_DIToSDLButton[i]))
        {
            g_ControllerData[i] = 0x80;
        }
    }

    return g_ControllerData;
}

u16 Controller::GetInput()
{
    u16 buttons = 0;

    const bool *keys = SDL_GetKeyboardState(NULL);

    buttons |= KEY_PRESSED(SDL_SCANCODE_UP, TH_BUTTON_UP);
    buttons |= KEY_PRESSED(SDL_SCANCODE_DOWN, TH_BUTTON_DOWN);
    buttons |= KEY_PRESSED(SDL_SCANCODE_LEFT, TH_BUTTON_LEFT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_RIGHT, TH_BUTTON_RIGHT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_8, TH_BUTTON_UP);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_2, TH_BUTTON_DOWN);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_4, TH_BUTTON_LEFT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_6, TH_BUTTON_RIGHT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_7, TH_BUTTON_UP_LEFT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_9, TH_BUTTON_UP_RIGHT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_1, TH_BUTTON_DOWN_LEFT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_KP_3, TH_BUTTON_DOWN_RIGHT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_HOME, TH_BUTTON_HOME);
    buttons |= KEY_PRESSED(SDL_SCANCODE_D, TH_BUTTON_D);
    buttons |= KEY_PRESSED(SDL_SCANCODE_Z, TH_BUTTON_SHOOT);
    buttons |= KEY_PRESSED(SDL_SCANCODE_X, TH_BUTTON_BOMB);
    buttons |= KEY_PRESSED(SDL_SCANCODE_LSHIFT, TH_BUTTON_FOCUS);
    buttons |= KEY_PRESSED(SDL_SCANCODE_RSHIFT, TH_BUTTON_FOCUS);
    buttons |= KEY_PRESSED(SDL_SCANCODE_ESCAPE, TH_BUTTON_MENU);
    buttons |= KEY_PRESSED(SDL_SCANCODE_LCTRL, TH_BUTTON_SKIP);
    buttons |= KEY_PRESSED(SDL_SCANCODE_RCTRL, TH_BUTTON_SKIP);
    buttons |= KEY_PRESSED(SDL_SCANCODE_Q, TH_BUTTON_Q);
    buttons |= KEY_PRESSED(SDL_SCANCODE_S, TH_BUTTON_S);
    buttons |= KEY_PRESSED(SDL_SCANCODE_R, TH_BUTTON_RESET);
    buttons |= KEY_PRESSED(SDL_SCANCODE_RETURN, TH_BUTTON_ENTER);

    return GetControllerInput(buttons) | Touch::GetButtonBits();
}

void Controller::ResetKeyboard()
{
    SDL_ResetKeyboard();
}
