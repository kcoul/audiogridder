/*
 * Copyright (c) 2020 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#if defined(__APPLE__)
#include "TargetConditionals.h"
#if (!TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)
#include <ApplicationServices/ApplicationServices.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include <iostream>
#include <utility>
#include <JuceHeader.h>

#include "KeyAndMouse.hpp"
#include "Utils.hpp"

namespace e47 {

setLogTagStatic("keyandmouse");

#if defined(JUCE_MAC)
void mouseEventInternal(CGMouseButton button, CGEventType type, CGPoint location, CGEventFlags flags) {
    traceScope();
    CGEventRef event = CGEventCreateMouseEvent(nullptr, type, location, button);
    CGEventSetType(event, type);
    CGEventSetFlags(event, flags | CGEventGetFlags(event));
    CGEventPost(kCGSessionEventTap, event);
    CFRelease(event);
}

void mouseDoubleClickEventInternal(CGPoint location, CGEventFlags flags) {
    traceScope();
    CGEventRef event = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, location, kCGMouseButtonLeft);
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, 2);
    CGEventSetFlags(event, flags | CGEventGetFlags(event));
    CGEventSetType(event, kCGEventLeftMouseDown);
    CGEventPost(kCGSessionEventTap, event);
    CGEventSetType(event, kCGEventLeftMouseUp);
    CGEventPost(kCGSessionEventTap, event);
    CFRelease(event);
}

void mouseScrollEventInternal(float deltaX, float deltaY) {
    traceScope();
    if (deltaX == 0 && deltaY == 0) {
        return;
    }
    CGEventRef event = nullptr;
    if (deltaX != 0) {
        event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, (int)lround(deltaY),
                                              (int)lround(deltaX));
    } else if (deltaY != 0) {
        event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 1, (int)lround(deltaY));
    }
    CGEventPost(kCGSessionEventTap, event);
    CFRelease(event);
}

void keyEventInternal(uint16_t keyCode, uint64_t flags, bool keyDown, bool currentProcessOnly) {
    traceScope();
    CGEventRef ev = CGEventCreateKeyboardEvent(nullptr, keyCode, keyDown);
    CGEventSetFlags(ev, flags);
    if (currentProcessOnly) {
        CGEventPostToPid(getpid(), ev);
    } else {
        CGEventPost(kCGSessionEventTap, ev);
    }
    CFRelease(ev);
}

inline std::pair<CGMouseButton, CGEventType> toMouseButtonType(MouseEvType t) {
    CGMouseButton button;
    CGEventType type;
    switch (t) {
        case MouseEvType::MOVE:
            button = kCGMouseButtonLeft;
            type = kCGEventMouseMoved;
            break;
        case MouseEvType::LEFT_UP:
            button = kCGMouseButtonLeft;
            type = kCGEventLeftMouseUp;
            break;
        case MouseEvType::LEFT_DOWN:
            button = kCGMouseButtonLeft;
            type = kCGEventLeftMouseDown;
            break;
        case MouseEvType::LEFT_DRAG:
            button = kCGMouseButtonLeft;
            type = kCGEventLeftMouseDragged;
            break;
        case MouseEvType::RIGHT_UP:
            button = kCGMouseButtonRight;
            type = kCGEventRightMouseUp;
            break;
        case MouseEvType::RIGHT_DOWN:
            button = kCGMouseButtonRight;
            type = kCGEventRightMouseDown;
            break;
        case MouseEvType::RIGHT_DRAG:
            button = kCGMouseButtonRight;
            type = kCGEventRightMouseDragged;
            break;
        case MouseEvType::OTHER_UP:
            button = kCGMouseButtonCenter;
            type = kCGEventOtherMouseUp;
            break;
        case MouseEvType::OTHER_DOWN:
            button = kCGMouseButtonCenter;
            type = kCGEventOtherMouseDown;
            break;
        case MouseEvType::OTHER_DRAG:
            button = kCGMouseButtonCenter;
            type = kCGEventOtherMouseDragged;
            break;
        case MouseEvType::WHEEL:
            button = kCGMouseButtonLeft;
            type = kCGEventNull;
            break;
        case MouseEvType::DBL_CLICK:
            break;
    }
    return std::make_pair(button, type);
}

#elif defined(JUCE_WINDOWS)

#define FLAG_VK_SHIFT 0x1
#define FLAG_VK_CONTROL 0x2
#define FLAG_VK_MENU 0x4

void sendInput(INPUT* in) {
    traceScope();
    if (SendInput(1, in, sizeof(INPUT)) != 1) {
        logln("SendInput failed: " << getLastErrorStr());
    }
}

void sendKey(WORD vk, bool keyDown, HWND hwnd = NULL) {
    traceScope();
    if (hwnd) {
        auto msg = keyDown ? WM_KEYDOWN : WM_KEYUP;
        if (SendMessage(hwnd, msg, vk, 1)) {
            logln("SendMessage failed: " << getLastErrorStr());
        }
    } else {
        INPUT event;
        event.type = INPUT_KEYBOARD;
        event.ki.wVk = vk;
        event.ki.wScan = 0;
        if (keyDown) {
            event.ki.dwFlags = 0;
        } else {
            event.ki.dwFlags = KEYEVENTF_KEYUP;
        }
        event.ki.time = 0;
        event.ki.dwExtraInfo = NULL;
        sendInput(&event);
    }
}

void mouseEventInternal(POINT pos, DWORD evFlags, uint64_t flags) {
    traceScope();

    INPUT event = {0};
    event.type = INPUT_MOUSE;
    event.mi.dx = pos.x;
    event.mi.dy = pos.y;
    event.mi.mouseData = 0;
    event.mi.time = 0;
    event.mi.dwExtraInfo = NULL;
    event.mi.dwFlags = evFlags;

    // modifiers down
    if ((flags & FLAG_VK_SHIFT) == FLAG_VK_SHIFT) {
        sendKey(VK_SHIFT, true);
    }
    if ((flags & FLAG_VK_CONTROL) == FLAG_VK_CONTROL) {
        sendKey(VK_CONTROL, true);
    }
    if ((flags & FLAG_VK_MENU) == FLAG_VK_MENU) {
        sendKey(VK_MENU, true);
    }

    sendInput(&event);

    // modifiers up
    if ((flags & FLAG_VK_SHIFT) == FLAG_VK_SHIFT) {
        sendKey(VK_SHIFT, false);
    }
    if ((flags & FLAG_VK_CONTROL) == FLAG_VK_CONTROL) {
        sendKey(VK_CONTROL, false);
    }
    if ((flags & FLAG_VK_MENU) == FLAG_VK_MENU) {
        sendKey(VK_MENU, false);
    }
}

void mouseScrollEventInternal(POINT pos, DWORD deltaX, DWORD deltaY) {
    traceScope();

    INPUT event = {0};
    event.type = INPUT_MOUSE;
    event.mi.time = 0;
    event.mi.dx = pos.x;
    event.mi.dy = pos.y;
    event.mi.dwExtraInfo = NULL;
    if (deltaX != 0) {
        event.mi.dwFlags = MOUSEEVENTF_HWHEEL;
        event.mi.mouseData = deltaX;
        sendInput(&event);
    }
    if (deltaY != 0) {
        event.mi.dwFlags = MOUSEEVENTF_WHEEL;
        event.mi.mouseData = deltaY;
        sendInput(&event);
    }
}

void keyEventInternal(WORD vk, uint64_t flags, bool keyDown, void* nativeHandle) {
    traceScope();

    auto hwnd = (HWND)nativeHandle;
    if (nullptr != nativeHandle && !IsWindow(hwnd)) {
        logln("nativeHandle is no HWND");
        return;
    }

    // modifiers down
    if (keyDown) {
        if ((flags & FLAG_VK_SHIFT) == FLAG_VK_SHIFT) {
            sendKey(VK_SHIFT, true, hwnd);
        }
        if ((flags & FLAG_VK_CONTROL) == FLAG_VK_CONTROL) {
            sendKey(VK_CONTROL, true, hwnd);
        }
        if ((flags & FLAG_VK_MENU) == FLAG_VK_MENU) {
            sendKey(VK_MENU, true, hwnd);
        }
    }

    // send key
    sendKey(vk, keyDown, hwnd);

    // modifiers up
    if (!keyDown) {
        if ((flags & FLAG_VK_SHIFT) == FLAG_VK_SHIFT) {
            sendKey(VK_SHIFT, false, hwnd);
        }
        if ((flags & FLAG_VK_CONTROL) == FLAG_VK_CONTROL) {
            sendKey(VK_CONTROL, false, hwnd);
        }
        if ((flags & FLAG_VK_MENU) == FLAG_VK_MENU) {
            sendKey(VK_MENU, false, hwnd);
        }
    }
}

inline POINT getScaledPoint(float x, float y) {
    traceScope();
    HDC hDC = GetDC(0);
    float dpi = (GetDeviceCaps(hDC, LOGPIXELSX) + GetDeviceCaps(hDC, LOGPIXELSY)) / 2.0f;
    ReleaseDC(0, hDC);
    if (auto* disp = Desktop::getInstance().getDisplays().getPrimaryDisplay()) {
        float sf = 96 / dpi;
        float xf = (float)0xffff / disp->totalArea.getWidth();
        float yf = (float)0xffff / disp->totalArea.getHeight();
        long lx = lroundf(x * sf * xf);
        long ly = lroundf(y * sf * yf);
        return {lx, ly};
    } else {
        return {lroundf(x), lroundf(y)};
    }
}

inline DWORD getMouseFlags(MouseEvType t) {
    DWORD flags = MOUSEEVENTF_ABSOLUTE;
    switch (t) {
        case MouseEvType::LEFT_DRAG:
        case MouseEvType::RIGHT_DRAG:
        case MouseEvType::OTHER_DRAG:
        case MouseEvType::MOVE:
            flags |= MOUSEEVENTF_MOVE;
            break;
        case MouseEvType::LEFT_UP:
            flags |= MOUSEEVENTF_LEFTUP;
            break;
        case MouseEvType::LEFT_DOWN:
            flags |= MOUSEEVENTF_LEFTDOWN;
            break;
        case MouseEvType::RIGHT_UP:
            flags |= MOUSEEVENTF_RIGHTUP;
            break;
        case MouseEvType::RIGHT_DOWN:
            flags |= MOUSEEVENTF_RIGHTDOWN;
            break;
        case MouseEvType::OTHER_UP:
            flags |= MOUSEEVENTF_MIDDLEUP;
            break;
        case MouseEvType::OTHER_DOWN:
            flags |= MOUSEEVENTF_MIDDLEDOWN;
            break;
    }
    return flags;
}

inline WORD getVK(uint16_t keyCode) {
    auto ch = getKeyName(keyCode);
    WORD vk = 0;
    if (ch.length() == 1) {
        vk = VkKeyScanExA(ch[0], GetKeyboardLayout(0));
    } else if (ch == "Space") {
        vk = VK_SPACE;
    } else if (ch == "Return") {
        vk = VK_RETURN;
    } else if (ch == "Backspace") {
        vk = VK_BACK;
    } else if (ch == "Escape") {
        vk = VK_ESCAPE;
    } else if (ch == "Delete") {
        vk = VK_DELETE;
    } else if (ch == "Home") {
        vk = VK_HOME;
    } else if (ch == "End") {
        vk = VK_END;
    } else if (ch == "PageUp") {
        vk = VK_PRIOR;
    } else if (ch == "PageDown") {
        vk = VK_NEXT;
    } else if (ch == "LeftArrow") {
        vk = VK_LEFT;
    } else if (ch == "RightArrow") {
        vk = VK_RIGHT;
    } else if (ch == "UpArrow") {
        vk = VK_UP;
    } else if (ch == "DownArrow") {
        vk = VK_DOWN;
    } else if (ch == "F1") {
        vk = VK_F1;
    } else if (ch == "F2") {
        vk = VK_F2;
    } else if (ch == "F3") {
        vk = VK_F3;
    } else if (ch == "F4") {
        vk = VK_F4;
    } else if (ch == "F5") {
        vk = VK_F5;
    } else if (ch == "F6") {
        vk = VK_F6;
    } else if (ch == "F7") {
        vk = VK_F7;
    } else if (ch == "F8") {
        vk = VK_F8;
    } else if (ch == "F9") {
        vk = VK_F9;
    } else if (ch == "F10") {
        vk = VK_F10;
    } else if (ch == "F11") {
        vk = VK_F11;
    } else if (ch == "F12") {
        vk = VK_F12;
    } else if (ch == "F13") {
        vk = VK_F13;
    } else if (ch == "F14") {
        vk = VK_F14;
    } else if (ch == "F15") {
        vk = VK_F15;
    } else if (ch == "F16") {
        vk = VK_F16;
    } else if (ch == "F17") {
        vk = VK_F17;
    } else if (ch == "F18") {
        vk = VK_F18;
    } else if (ch == "F19") {
        vk = VK_F19;
    } else if (ch == "F20") {
        vk = VK_F20;
    } else if (ch == "Numpad0") {
        vk = VK_NUMPAD0;
    } else if (ch == "Numpad1") {
        vk = VK_NUMPAD1;
    } else if (ch == "Numpad2") {
        vk = VK_NUMPAD2;
    } else if (ch == "Numpad3") {
        vk = VK_NUMPAD3;
    } else if (ch == "Numpad4") {
        vk = VK_NUMPAD4;
    } else if (ch == "Numpad5") {
        vk = VK_NUMPAD5;
    } else if (ch == "Numpad6") {
        vk = VK_NUMPAD6;
    } else if (ch == "Numpad7") {
        vk = VK_NUMPAD7;
    } else if (ch == "Numpad8") {
        vk = VK_NUMPAD8;
    } else if (ch == "Numpad9") {
        vk = VK_NUMPAD9;
    } else if (ch == "Numpad*") {
        vk = VK_MULTIPLY;
    } else if (ch == "Numpad/") {
        vk = VK_DIVIDE;
    } else if (ch == "Numpad+") {
        vk = VK_ADD;
    } else if (ch == "Numpad-") {
        vk = VK_SUBTRACT;
    } else if (ch == "Numpad.") {
        vk = VK_DECIMAL;
    } else if (ch == "Numpad=") {
        vk = 0x92;
    } else if (ch == "NumpadClear") {
        vk = VK_DELETE;
    }
    return vk;
}

#elif defined(JUCE_LINUX)

#include <X11/extensions/XTest.h>
// #include <X11/XKBlib.h>

#define FLAG_VK_SHIFT 0x1
#define FLAG_VK_CONTROL 0x2
#define FLAG_VK_MENU 0x4

void mouseDoubleClickEventInternal(XPoint /* location */) {
    traceScope();
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        logln("Failed to open Display \n");
        return;
    }
    /* Fake the mouse button Press and Release events */
    XTestFakeButtonEvent(display, Button1, True, CurrentTime);
    XTestFakeButtonEvent(display, Button1, False, CurrentTime);
    XTestFakeButtonEvent(display, Button1, True, CurrentTime);
    XTestFakeButtonEvent(display, Button1, False, CurrentTime);
    XCloseDisplay(display);
}
/*
| Physical Button | Button Event | normal action |
|-----------------|--------------|---------------|
|Left             |            1 |        select |
|Middle           |            2 | paste/depends |
|Right            |            3 |  context menu |
|Scroll Up        |            4 |  context menu |
|Scroll Down      |            5 |  context menu |
|Custom           |           6+ |       depends |
*/

void mouseEventInternal(MouseEvType t, XPoint pos, uint64_t /* flags */) {
    traceScope();
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        logln("Failed to open Display \n");
        return;
    }
    switch (t) {
        case MouseEvType::LEFT_DRAG:
        case MouseEvType::RIGHT_DRAG:
        case MouseEvType::OTHER_DRAG:
        case MouseEvType::MOVE:
            XTestFakeMotionEvent(display, -1, pos.x, pos.y, CurrentTime);
            break;
        case MouseEvType::LEFT_UP:
            XTestFakeButtonEvent(display, Button1, False, CurrentTime);
            break;
        case MouseEvType::LEFT_DOWN:
            XTestFakeButtonEvent(display, Button1, True, CurrentTime);
            break;
        case MouseEvType::RIGHT_UP:
            XTestFakeButtonEvent(display, Button3, False, CurrentTime);
            break;
        case MouseEvType::RIGHT_DOWN:
            XTestFakeButtonEvent(display, Button3, True, CurrentTime);
            break;
        case MouseEvType::OTHER_UP:
            XTestFakeButtonEvent(display, Button2, False, CurrentTime);
            break;
        case MouseEvType::OTHER_DOWN:
            XTestFakeButtonEvent(display, Button2, True, CurrentTime);
            break;
        default:
            break;
    }

    XCloseDisplay(display);
}
void mouseScrollEventInternal(XPoint /* pos */, float /* deltaX */, float /* deltaY */) {}
void keyEventInternal(uint16_t keyCode, uint64_t /*flags*/, bool keyDown, void* /*nativeHandle*/) {
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        logln("Failed to open Display \n");
        return;
    }
    uint16_t modcode = XKeysymToKeycode(display, XStringToKeysym(getKeyName(keyCode).c_str()));
    XTestFakeKeyEvent(display, modcode, keyDown, CurrentTime);
    XCloseDisplay(display);
}
inline XPoint getScaledPoint(float x, float y) { return {(short)roundf(x), (short)roundf(y)}; }

#endif

void mouseEvent(MouseEvType t, float x, float y, uint64_t flags) {
#if defined(JUCE_MAC)
    if (t == MouseEvType::DBL_CLICK) {
        CGPoint loc = CGPointMake(x, y);
        mouseDoubleClickEventInternal(loc, flags);
    } else {
        auto bt = toMouseButtonType(t);
        CGPoint loc = CGPointMake(x, y);
        mouseEventInternal(bt.first, bt.second, loc, flags);
    }
#elif defined(JUCE_WINDOWS)
    auto pos = getScaledPoint(x, y);
    if (t == MouseEvType::DBL_CLICK) {
        // No dedicated handling needed, two down/up events will appear which will be detected as double click
    } else {
        auto mouseFlags = getMouseFlags(t);
        mouseEventInternal(pos, mouseFlags, flags);
    }
#elif defined(JUCE_LINUX)
    XPoint loc = {(short int)x, (short int)y};
    if (t == MouseEvType::DBL_CLICK) {
        mouseDoubleClickEventInternal(loc);
    } else {
        mouseEventInternal(t, loc, flags);
    }
#endif
}

void mouseScrollEvent(float x, float y, float deltaX, float deltaY, bool isSmooth) {
#if defined(JUCE_MAC)
    ignoreUnused(x);
    ignoreUnused(y);

    if (isSmooth) {
        const float scale = 0.5f / 256.0f;
        mouseScrollEventInternal(deltaX / scale, deltaY / scale);
    } else {
        const float scale = 10.0f / 256.0f;
        mouseScrollEventInternal(deltaX / scale, deltaY / scale);
    }
#elif defined(JUCE_WINDOWS)
    ignoreUnused(isSmooth);

    auto pos = getScaledPoint(x, y);
    mouseScrollEventInternal(pos, lround(deltaX * 512), lround(deltaY * 512));
#elif defined(JUCE_LINUX)
    XPoint loc = {(short int)x, (short int)y};
    ignoreUnused(isSmooth);
    mouseScrollEventInternal(loc, deltaX, deltaY);
#endif
}

void keyEvent(uint16_t keyCode, uint64_t flags, bool keyDown, bool currentProcessOnly, void* nativeHandle) {
#if defined(JUCE_MAC)
    ignoreUnused(nativeHandle);
    keyEventInternal(keyCode, flags, keyDown, currentProcessOnly);
#elif defined(JUCE_WINDOWS)
    keyEventInternal(getVK(keyCode), flags, keyDown, currentProcessOnly ? nativeHandle : nullptr);
#elif defined(JUCE_LINUX)
    keyEventInternal(keyCode, flags, keyDown, currentProcessOnly ? nativeHandle : nullptr);
#endif
}

void setShiftKey(uint64_t& flags) {
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskShift;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_SHIFT;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_SHIFT;
#endif
}

void setControlKey(uint64_t& flags) {
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskControl;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_CONTROL;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_CONTROL;
#endif
}

void setAltKey(uint64_t& flags) {
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskAlternate;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_MENU;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_MENU;
#endif
}

void setCopyKeys(uint16_t& key, uint64_t& flags) {
    key = getKeyCode("C");
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskCommand;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_CONTROL;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_CONTROL;
#endif
}

void setPasteKeys(uint16_t& key, uint64_t& flags) {
    key = getKeyCode("V");
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskCommand;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_CONTROL;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_CONTROL;
#endif
}

void setCutKeys(uint16_t& key, uint64_t& flags) {
    key = getKeyCode("X");
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskCommand;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_CONTROL;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_CONTROL;
#endif
}

void setSelectAllKeys(uint16_t& key, uint64_t& flags) {
    key = getKeyCode("A");
#if defined(JUCE_MAC)
    flags |= kCGEventFlagMaskCommand;
#elif defined(JUCE_WINDOWS)
    flags |= FLAG_VK_CONTROL;
#elif defined(JUCE_LINUX)
    flags |= FLAG_VK_CONTROL;
#endif
}

void keyEventDown(uint16_t keyCode, uint64_t flags, bool currentProcessOnly, void* nativeHandle) {
    keyEvent(keyCode, flags, true, currentProcessOnly, nativeHandle);
}

void keyEventUp(uint16_t keyCode, uint64_t flags, bool currentProcessOnly, void* nativeHandle) {
    keyEvent(keyCode, flags, false, currentProcessOnly, nativeHandle);
}

}  // namespace e47
