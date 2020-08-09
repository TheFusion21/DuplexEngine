#pragma once

#define SAFEDELETE(x) { if(x) { delete (x); (x)=nullptr; }}
#define SAFERELEASE(x) { if(x) { (x)->Release(); (x)=nullptr; }}
#define SAFEDELETEARR(x) { if(x) { delete[] (x); (x)=nullptr; }}
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))