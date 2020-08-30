#pragma once
#include <stdint.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;

#define UI8MAX UCHAR_MAX
#define UI16MAX USHRT_MAX
#define UI32MAX UINT_MAX
#define UI64MAX ULLONG_MAX

typedef unsigned char byte;
typedef const char* AnsiString;
typedef const wchar_t* WideString;
typedef void* voidptr;

#ifdef DOUBLEPRECISION
typedef double real;
#else
typedef float real;
#endif
typedef int BOOL;

typedef int* GraphicsBufferPtr;
typedef int* ShaderResourcePtr;
typedef int* IntPtr;
