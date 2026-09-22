#pragma once
#include "math/mat4x4.h"
#include "utils/color.h"

#define MAX_LIGHTS 8

namespace DUPLEX_NS_UTIL
{
    struct alignas(16) GpuLight
    {
        ui32 type; // 4 bytes
        DUPLEX_NS_MATH::Vec3 color = DUPLEX_NS_MATH::Vec3UnitScale; // 3*4 -> 12 bytes

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

        DUPLEX_NS_MATH::Mat4x4 transform = DUPLEX_NS_MATH::Mat4x4Identity; //4*4*4 bytes -> 64 bytes

        DUPLEX_NS_MATH::Vec3 position; // 3*4 -> 12 bytes
    };
    struct alignas(16) modelConstant
    {
        DUPLEX_NS_MATH::Mat4x4 world = DUPLEX_NS_MATH::Mat4x4Identity; // 64 bytes
    };

    struct alignas(16) worldConstant
    {
        GpuLight lights[MAX_LIGHTS]; //4+12+4+4+4+4+4+4+4+8+64+12 -> 128 bytes

        DUPLEX_NS_MATH::Mat4x4 projView = DUPLEX_NS_MATH::Mat4x4Identity;//4*4*4 bytes -> 64 bytes

        DUPLEX_NS_MATH::Vec3 eye = DUPLEX_NS_MATH::Vec3Zero;// 3*4 -> 12 bytes

        ui32 lightCount = 0; // 4 bytes
    };
}