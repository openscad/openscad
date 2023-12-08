#pragma once
#if __has_include(<tracy/Tracy.hpp>)
#include <tracy/Tracy.hpp>
#else
#define FrameMarkStart(x)
#define FrameMarkEnd(x)
#define ZoneScoped
#endif
