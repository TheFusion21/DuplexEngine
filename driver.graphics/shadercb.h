#pragma once
#include "math/mat4x4.h"
#include "utils/color.h"

#define MAX_LIGHTS 8

namespace DUPLEX_NS_UTIL
{
    __declspec(align(16)) struct GpuLight
    {
        ui32 type; // 4 bytes
        DUPLEX_NS_MATH::Vec3 color = DUPLEX_NS_MATH::Vec3::UnitScale; // 3*4 -> 12 bytes

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

        DUPLEX_NS_MATH::Mat4x4 transform = DUPLEX_NS_MATH::Mat4x4::Identity; //4*4*4 bytes -> 64 bytes

        DUPLEX_NS_MATH::Vec3 position; // 3*4 -> 12 bytes
    };
    __declspec(align(16)) struct modelConstant
    {
        DUPLEX_NS_MATH::Mat4x4 world = DUPLEX_NS_MATH::Mat4x4::Identity; // 64 bytes
    };

    __declspec(align(16)) struct worldConstant
    {
        GpuLight lights[MAX_LIGHTS]; //4+12+4+4+4+4+4+4+4+8+64+12 -> 128 bytes

        DUPLEX_NS_MATH::Mat4x4 projView = DUPLEX_NS_MATH::Mat4x4::Identity;//4*4*4 bytes -> 64 bytes

        DUPLEX_NS_MATH::Vec3 eye = DUPLEX_NS_MATH::Vec3::Zero;// 3*4 -> 12 bytes

        ui32 lightCount = 0; // 4 bytes
    };
}