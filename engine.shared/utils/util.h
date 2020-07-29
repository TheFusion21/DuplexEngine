#pragma once

namespace Engine::Utils
{
#define SAFEDELETE(x) { if(x) { delete (x); (x)=nullptr; }}
#define SAFERELEASE(x) { if(x) { (x)->Release(); (x)=nullptr; }}
#define SAFEDELETEARR(x) { if(x) { delete[] (x); (x)=nullptr; }}
}