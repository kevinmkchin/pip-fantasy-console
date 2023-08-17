#include "InputSystem.h"

#include "MesaCommon.h"

InputSystem Input;

//////////////////////////////////////////////////////////////////////
// GENERIC CLASSES AND FUNCTIONS BELOW
//////////////////////////////////////////////////////////////////////

/** Global **/
void InputSystem::ProcessAllSDLInputEvents(const SDL_Event event)
{
    switch (event.type)
    {
        case SDL_MOUSEBUTTONUP: {
            ProcessSDLMouseButtonEvent(event.button);
        }break;
        case SDL_MOUSEBUTTONDOWN:{
            ProcessSDLMouseButtonEvent(event.button);
        }break;
        case SDL_MOUSEMOTION: {
            ProcessSDLMouseMotionEvent(event.motion);
        }break;
        case SDL_KEYDOWN: {
            ProcessSDLKeyDownEvent(event.key);
        }break;
        case SDL_KEYUP: {
            ProcessSDLKeyUpEvent(event.key);
        }break;
        case SDL_CONTROLLERBUTTONDOWN: {
            ProcessSDLControllerButtonDownEvent(event.cbutton);
        }break;
        case SDL_CONTROLLERBUTTONUP: {
            ProcessSDLControllerButtonUpEvent(event.cbutton);
        }break;
        case SDL_CONTROLLERAXISMOTION: {
            ProcessSDLControllerAxisEvent(event.caxis);
        }break;
        case SDL_CONTROLLERDEVICEADDED: {
            ProcessSDLControllerConnected(event.cdevice.which);
        }break;
        case SDL_CONTROLLERDEVICEREMOVED: {
            ProcessSDLControllerRemoved(event.cdevice.which);
        }break;
    }
}

void InputSystem::ResetInputStatesAtEndOfFrame()
{
    mouseDelta = vec2(0.f, 0.f);

    mouseLeftHasBeenPressed = false;
    mouseRightHasBeenPressed = false;
    mouseMiddleHasBeenPressed = false;
    mouseLeftHasBeenReleased = false;
    mouseRightHasBeenReleased = false;
    mouseMiddleHasBeenReleased = false;

    memset(justPressedKeyState, 0, 256);
    memset(justReleasedKeyState, 0, 256);

    for (int i = 0; i < MAX_GAMEPAD_COUNT; ++i) {
        gamepadStates[i].justPressedButtonState = 0;
        gamepadStates[i].justReleasedButtonState = 0;
    }
}

bool InputSystem::KeyPressed(SDL_Scancode scancode)
{
    return currentKeyState[scancode];
}

bool InputSystem::KeyReleased(SDL_Scancode scancode)
{
    return !KeyPressed(scancode);
}

bool InputSystem::KeyHasBeenPressed(SDL_Scancode scancode)
{
    return justPressedKeyState[scancode];
}

bool InputSystem::KeyHasBeenReleased(SDL_Scancode scancode)
{
    return justReleasedKeyState[scancode];
}

void InputSystem::ProcessSDLMouseButtonEvent(SDL_MouseButtonEvent mouseButtonEvent)
{
    if (mouseButtonEvent.button == SDL_BUTTON_LEFT)
    {
        static bool lmbLastFrameDown = false;
        mouseLeftPressed = mouseButtonEvent.type == SDL_MOUSEBUTTONDOWN;
        if (!lmbLastFrameDown && mouseLeftPressed) mouseLeftHasBeenPressed = true;
        if (lmbLastFrameDown && !mouseLeftPressed) mouseLeftHasBeenReleased = true;
        lmbLastFrameDown = mouseLeftPressed;
    }
    if (mouseButtonEvent.button == SDL_BUTTON_RIGHT)
    {
        static bool rmbLastFrameDown = false;
        mouseRightPressed = mouseButtonEvent.type == SDL_MOUSEBUTTONDOWN;
        if (!rmbLastFrameDown && mouseRightPressed) mouseRightHasBeenPressed = true;
        if (rmbLastFrameDown && !mouseRightPressed) mouseRightHasBeenReleased = true;
        rmbLastFrameDown = mouseRightPressed;
    }
    if (mouseButtonEvent.button == SDL_BUTTON_MIDDLE)
    {
        static bool mmbLastFrameDown = false;
        mouseMiddlePressed = mouseButtonEvent.type == SDL_MOUSEBUTTONDOWN;
        if (!mmbLastFrameDown && mouseMiddlePressed) mouseMiddleHasBeenPressed = true;
        if (mmbLastFrameDown && !mouseMiddlePressed) mouseMiddleHasBeenReleased = true;
        mmbLastFrameDown = mouseMiddlePressed;
    }
}

void InputSystem::ProcessSDLMouseMotionEvent(SDL_MouseMotionEvent mouseMotionEvent)
{
    mouseDelta = vec2((float)mouseMotionEvent.xrel, (float)mouseMotionEvent.yrel);
    mousePos = ivec2(mouseMotionEvent.x, mouseMotionEvent.y);
}

void InputSystem::ProcessSDLKeyDownEvent(SDL_KeyboardEvent keyEvent) {
    SDL_Scancode key = keyEvent.keysym.scancode;
    currentKeyState[key] = 1;
    if (!keyEvent.repeat) {
        justPressedKeyState[key] = 1;
    }
}

void InputSystem::ProcessSDLKeyUpEvent(SDL_KeyboardEvent keyEvent) {
    // Never set JustPressed or JustReleased to 0 here.
    SDL_Scancode key = keyEvent.keysym.scancode;
    currentKeyState[key] = 0;
    justReleasedKeyState[key] = 1;
}


GamepadState& InputSystem::GetGamepad(i32 playerNumber)
{
    return gamepadStates[playerNumber];
}

void InputSystem::ProcessSDLControllerButtonDownEvent(SDL_ControllerButtonEvent gamepadButtonEvent) {
    /* SDL_GameControllerButton reference:
    SDL_CONTROLLER_BUTTON_A 0
    SDL_CONTROLLER_BUTTON_B 1
    SDL_CONTROLLER_BUTTON_X 2
    SDL_CONTROLLER_BUTTON_Y 3
    SDL_CONTROLLER_BUTTON_BACK 4
    SDL_CONTROLLER_BUTTON_GUIDE 5
    SDL_CONTROLLER_BUTTON_START 6
    SDL_CONTROLLER_BUTTON_LEFTSTICK 7
    SDL_CONTROLLER_BUTTON_RIGHTSTICK 8
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER 9
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 10
    SDL_CONTROLLER_BUTTON_DPAD_UP 11
    SDL_CONTROLLER_BUTTON_DPAD_DOWN 12
    SDL_CONTROLLER_BUTTON_DPAD_LEFT 13
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT 14
    */

    i32 joystickInstanceID = gamepadButtonEvent.which;
    i32 gamepadIndex = gamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 buttonIndex = gamepadButtonEvent.button;

    switch (buttonIndex) {
        case 0: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_A;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_A;
        }break;
        case 1: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_B;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_B;
        }break;
        case 2: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_X;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_X;
        }break;
        case 3: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_Y;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_Y;
        }break;
        case 4: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_BACK;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_BACK;
        }break;
        case 6: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_START;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_START;
        }break;
        case 7: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_LTHUMB;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_LTHUMB;
        }break;
        case 8: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_RTHUMB;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_RTHUMB;
        }break;
        case 9: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_LB;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_LB;
        }break;
        case 10: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_RB;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_RB;
        }break;
        case 11: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_UP;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_UP;
        }break;
        case 12: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_DOWN;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_DOWN;
        }break;
        case 13: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_LEFT;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_LEFT;
        }break;
        case 14: {
            gamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_RIGHT;
            gamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_RIGHT;
        }break;
    }
}

void InputSystem::ProcessSDLControllerButtonUpEvent(SDL_ControllerButtonEvent gamepadButtonEvent) {
    i32 joystickInstanceID = gamepadButtonEvent.which;
    i32 gamepadIndex = gamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 buttonIndex = gamepadButtonEvent.button;

    switch (buttonIndex) {
        case 0: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_A;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_A;
        }break;
        case 1: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_B;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_B;
        }break;
        case 2: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_X;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_X;
        }break;
        case 3: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_Y;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_Y;
        }break;
        case 4: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_BACK;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_BACK;
        }break;
        case 6: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_START;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_START;
        }break;
        case 7: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_LTHUMB;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_LTHUMB;
        }break;
        case 8: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_RTHUMB;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_RTHUMB;
        }break;
        case 9: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_LB;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_LB;
        }break;
        case 10: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_RB;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_RB;
        }break;
        case 11: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_UP;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_UP;
        }break;
        case 12: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_DOWN;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_DOWN;
        }break;
        case 13: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_LEFT;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_LEFT;
        }break;
        case 14: {
            gamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_RIGHT;
            gamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_RIGHT;
        }break;
    }
}

void InputSystem::ProcessSDLControllerAxisEvent(SDL_ControllerAxisEvent gamepadAxisEvent) {
    i32 joystickInstanceID = gamepadAxisEvent.which;
    i32 gamepadIndex = gamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 axisIndex = gamepadAxisEvent.axis;
    i16 axisValue = gamepadAxisEvent.value;

    switch (axisIndex) {
        case SDL_CONTROLLER_AXIS_LEFTX: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                vec2 leftAxisDir = gamepadStates[gamepadIndex].leftThumbStickDir * (float)SDL_JOYSTICK_MAX;
                float oldX = leftAxisDir.x;
                leftAxisDir.x = (float)axisValue;
                leftAxisDir /= (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].leftThumbStickDir = leftAxisDir;

                vec2 leftAxisDelta = gamepadStates[gamepadIndex].leftThumbStickDelta;
                leftAxisDelta.x = leftAxisDir.x - oldX;
                gamepadStates[gamepadIndex].leftThumbStickDelta = leftAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
            else {
                float oldX = gamepadStates[gamepadIndex].leftThumbStickDir.x * (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].leftThumbStickDir.x = 0.f;

                vec2 leftAxisDelta = gamepadStates[gamepadIndex].leftThumbStickDelta;
                leftAxisDelta.x = -oldX;
                gamepadStates[gamepadIndex].leftThumbStickDelta = leftAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
        }break;
        case SDL_CONTROLLER_AXIS_LEFTY: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                vec2 leftAxisDir = gamepadStates[gamepadIndex].leftThumbStickDir * (float)SDL_JOYSTICK_MAX;
                float oldY = leftAxisDir.y;
                leftAxisDir.y = (float)(invertLeftYAxis ? -axisValue : axisValue);
                leftAxisDir /= (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].leftThumbStickDir = leftAxisDir;

                vec2 leftAxisDelta = gamepadStates[gamepadIndex].leftThumbStickDelta;
                leftAxisDelta.y = leftAxisDir.y - oldY;
                gamepadStates[gamepadIndex].leftThumbStickDelta = leftAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
            else {
                float oldY = gamepadStates[gamepadIndex].leftThumbStickDir.y * (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].leftThumbStickDir.y = 0.f;

                vec2 leftAxisDelta = gamepadStates[gamepadIndex].leftThumbStickDelta;
                leftAxisDelta.y = -oldY;
                gamepadStates[gamepadIndex].leftThumbStickDelta = leftAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
        }break;
        case SDL_CONTROLLER_AXIS_RIGHTX: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                vec2 rightAxisDir = gamepadStates[gamepadIndex].rightThumbStickDir * (float)SDL_JOYSTICK_MAX;
                float oldX = rightAxisDir.x;
                rightAxisDir.x = (float)axisValue;
                rightAxisDir /= (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].rightThumbStickDir = rightAxisDir;

                vec2 rightAxisDelta = gamepadStates[gamepadIndex].rightThumbStickDelta;
                rightAxisDelta.x = rightAxisDir.x - oldX;
                gamepadStates[gamepadIndex].leftThumbStickDelta = rightAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
            else {
                float oldX = gamepadStates[gamepadIndex].rightThumbStickDir.x * (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].rightThumbStickDir.x = 0.f;

                vec2 rightAxisDelta = gamepadStates[gamepadIndex].rightThumbStickDelta;
                rightAxisDelta.x = -oldX;
                gamepadStates[gamepadIndex].rightThumbStickDelta = rightAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
        }break;
        case SDL_CONTROLLER_AXIS_RIGHTY: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                vec2 rightAxisDir = gamepadStates[gamepadIndex].rightThumbStickDir * (float)SDL_JOYSTICK_MAX;
                float oldY = rightAxisDir.y;
                rightAxisDir.y = (float)(invertRightYAxis ? -axisValue : axisValue);
                rightAxisDir /= (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].rightThumbStickDir = rightAxisDir;

                vec2 rightAxisDelta = gamepadStates[gamepadIndex].rightThumbStickDelta;
                rightAxisDelta.y = rightAxisDir.y - oldY;
                gamepadStates[gamepadIndex].leftThumbStickDelta = rightAxisDelta;
            }
            else {
                float oldY = gamepadStates[gamepadIndex].rightThumbStickDir.y * (float)SDL_JOYSTICK_MAX;
                gamepadStates[gamepadIndex].rightThumbStickDir.y = 0.f;

                vec2 rightAxisDelta = gamepadStates[gamepadIndex].rightThumbStickDelta;
                rightAxisDelta.y = -oldY;
                gamepadStates[gamepadIndex].rightThumbStickDelta = rightAxisDelta / (float)SDL_JOYSTICK_MAX;
            }
        }break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                gamepadStates[gamepadIndex].leftTrigger =
                        (axisValue + ((float)SDL_JOYSTICK_MAX)) / (2 * ((float)SDL_JOYSTICK_MAX));
            }
            else {
                gamepadStates[gamepadIndex].leftTrigger = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: {
            if (GM_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE) {
                gamepadStates[gamepadIndex].rightTrigger =
                        (axisValue + ((float)SDL_JOYSTICK_MAX)) / (2 * ((float)SDL_JOYSTICK_MAX));
            }
            else {
                gamepadStates[gamepadIndex].rightTrigger = 0.f;
            }
        }break;
    }
}

void InputSystem::ProcessSDLControllerConnected(i32 deviceIndex) {
    SDL_GameController* deviceHandle = SDL_GameControllerOpen(deviceIndex);
    i32 joystickInstanceID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(deviceHandle));
    gamepadStates.OnConnect(joystickInstanceID);

    printf("Gamepad connected. Instance ID: %d\n", joystickInstanceID);
}

void InputSystem::ProcessSDLControllerRemoved(i32 joystickInstanceID) {
    gamepadStates.OnDisconnect(joystickInstanceID);
    SDL_GameControllerClose(SDL_GameControllerFromInstanceID(joystickInstanceID));

    printf("Gamepad disconnected. Instance ID: %d\n", joystickInstanceID);
}


bool GamepadState::IsPressed(u16 button /* bitwise-OR-able */) const {
    return currentButtonState & button;
}

bool GamepadState::IsReleased(u16 button /* bitwise-OR-able */) const {
    return !IsPressed(button);
}

bool GamepadState::HasBeenPressed(u16 button) const {
    return justPressedButtonState & button;
}

bool GamepadState::HasBeenReleased(u16 button) const {
    return justReleasedButtonState & button;
}


GamepadState& GamepadStatesWrapper::AtInstanceID(const i32 instanceID) {
    return states[mapInstanceIDtoGamepadIndex[instanceID]];
}

u8 GamepadStatesWrapper::GamepadIndexFromInstanceID(const i32 instanceID) {
    return mapInstanceIDtoGamepadIndex[instanceID];
}

void GamepadStatesWrapper::OnConnect(i32 instanceID) {
    mapInstanceIDtoGamepadIndex[instanceID] = gamepadCount;
    states[gamepadCount].isConnected = true;
    ++gamepadCount;
}

void GamepadStatesWrapper::OnDisconnect(i32 instanceID) {
    i32 gamepadIndex = mapInstanceIDtoGamepadIndex[instanceID];

    // shift game states left
    for (i32 i = gamepadIndex; i < gamepadCount - 1; ++i) {
        states[i] = states[i + 1];
    }

    // reset last game state. we shift these over so just resetting the last is enough.
    states[gamepadCount - 1].isConnected = false;
    states[gamepadCount - 1].currentButtonState = 0;
    states[gamepadCount - 1].justPressedButtonState = 0;
    states[gamepadCount - 1].justReleasedButtonState = 0;
    states[gamepadCount - 1].leftTrigger = 0.f;
    states[gamepadCount - 1].rightTrigger = 0.f;
    states[gamepadCount - 1].leftThumbStickDir = { 0.f, 0.f };
    states[gamepadCount - 1].leftThumbStickDir = { 0.f, 0.f };

    // shift indices left
    for (int i = 0; i < 128; ++i) {
        if (gamepadIndex < mapInstanceIDtoGamepadIndex[i]) {
            --mapInstanceIDtoGamepadIndex[i];
        }
    }

    --gamepadCount;
}
