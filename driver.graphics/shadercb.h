#pragma once
#include "math/mat4x4.h"
#include "utils/color.h"

#define MAX_LIGHTS 256

namespace Engine::Utils
{
    struct Light
    {
        Engine::Math::Vec3 color = Engine::Math::Vec3::Zero;
        float falloffStart = 0.0f;
        Engine::Math::Vec3 direction = Engine::Math::Vec3::Zero;
        float fallOffEnd = 0.0f;
        Engine::Math::Vec3 position = Engine::Math::Vec3::Zero;
        float spotPower = 0.0f;

    };
    struct modelConstant
    {
        Engine::Math::Mat4x4 world = Engine::Math::Mat4x4::Identity; // 64 bytes
        //Material m; //52 bytes

        //Engine::Math::Vec3 buffer; //3 * 4 bytes
    };

    struct worldConstant
    {
        Engine::Math::Mat4x4 projView = Engine::Math::Mat4x4::Identity;
        Engine::Math::Vec3 eye = Engine::Math::Vec3::Zero;
        float ambientLight = 0.2f;
    };
}