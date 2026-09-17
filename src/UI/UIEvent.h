#pragma once

#include "Core/Math.h"

enum class UIEventType {
    None,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput
};

struct UIEvent {
    UIEventType type = UIEventType::None;
    Vec2f pos{0, 0};        // mouse pos
    int button = 0;          // 0 = left, 1 = right, 2 = middle
    float wheel = 0.0f;
    int key = 0;
    char text[32] = {};      // TextInput
};
