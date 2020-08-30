#pragma once

#define SAFEDELETE(x) { if(x) { delete (x); (x)=nullptr; }}
#define SAFERELEASE(x) { if(x) { (x)->Release(); (x)=nullptr; }}
#define SAFEDELETEARR(x) { if(x) { delete[] (x); (x)=nullptr; }}
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(WIN32) || defined(_WIN32)
#define FORCEINLINE __forceinline
#define FORCENOINLINE _declspec(noinline)
#elif defined(__linux__) || defined(__APPLE__)
#define FORCEINLINE inline
#endif

#define ASSERTIONS_ENABLED

#ifdef ASSERTIONS_ENABLED
    #include <iostream>

    #if _MSC_VER
    #include <intrin.h>
    #define debugBreak() __debugbreak();
    #else
    #define debugBreak() __asm { int 3 }
    #endif

    #define ASSERT(expr) { \
        if( expr ) { \
        } else { \
            reportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            debugBreak(); \
        } \
    }

    #define ASSERT_MSG(expr, message) { \
        if( expr ) { \
        } else { \
            reportAssertionFailure(#expr, message, __FILE__, __LINE__); \
            debugBreak(); \
        } \
    }

    #ifdef _DEBUG
    #define ASSERT_DEBUG(expr) { \
        if( expr ) { \
        } else { \
            reportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            debugBreak(); \
        } \
    }
    #else
    #define ASSERT_DEBUG(expr) // Does nothing at all
    #endif

    FORCEINLINE void reportAssertionFailure(const char* expression, const char* message, const char* file, int line) {
        std::cerr << "Assertion Failure: " << expression << ", message: '" << message << "', in file: " << file << ", line: " << line << "\n";
    }

#else
    #define ASSERT(expr) // Does nothing at all
    #define ASSERT_MSG(expr, message) // Does nothing at all
    #define ASSERT_DEBUG(expr) // Does nothing at all
#endif