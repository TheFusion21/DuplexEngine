#pragma once
#include "math/mat4x4.h"
#include "utils/color.h"

#define MAX_LIGHTS 8

namespace Engine::Utils
{
    __declspec(align(16)) struct GpuLight
    {
        ui32 type; // 4 bytes
        Engine::Math::Vec3 color = Engine::Math::Vec3::UnitScale; // 3*4 -> 12 bytes

        float intensity = 1; // 4 bytes

        float indirectMul = 1; // 4 bytes

        //DL
        float angularDiameter = 1; // 4 bytes


        //SL
        float outerAngle = 0; // 4 bytes
        float innerAngle = 0; // 4 bytes

        //P
        float radius = 0; // 4 bytes

        float range = 0; // 4 bytes

        Engine::Math::Mat4x4 transform = Engine::Math::Mat4x4::Identity; //4*4*4 bytes -> 64 bytes

        Engine::Math::Vec3 position; // 3*4 -> 12 bytes
    };
    __declspec(align(16)) struct modelConstant
    {
        Engine::Math::Mat4x4 world = Engine::Math::Mat4x4::Identity; // 64 bytes
    };

    __declspec(align(16)) struct worldConstant
    {
        GpuLight lights[MAX_LIGHTS]; //4+12+4+4+4+4+4+4+4+8+64+12 -> 128 bytes

        Engine::Math::Mat4x4 projView = Engine::Math::Mat4x4::Identity;//4*4*4 bytes -> 64 bytes

        Engine::Math::Vec3 eye = Engine::Math::Vec3::Zero;// 3*4 -> 12 bytes

        ui32 lightCount = 0; // 4 bytes
    };
}