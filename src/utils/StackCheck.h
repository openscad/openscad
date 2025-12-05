#pragma once

#include <cstdlib>
#include <cstdint>
#include "platform/PlatformUtils.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26486)  // Disable warning for dangling pointers
#endif                            // defined(_MSC_VER)

class StackCheck
{
public:
  static StackCheck& inst()
  {
    static StackCheck instance;
    return instance;
  }

  inline bool check() { return size() >= limit; }

private:
  StackCheck() : limit(PlatformUtils::stackLimit())
  {
    unsigned char c;
    base = reinterpret_cast<std::uintptr_t>(&c);  // NOLINT(*reinterpret-cast, *StackAddressEscape)
  }

  // Use size_t instead of unsigned long to avoid truncation on Windows x64
  // where unsigned long is 32-bit but pointers are 64-bit.
  inline size_t size() const
  {
    unsigned char c;
    const auto current = reinterpret_cast<std::uintptr_t>(&c);
    // Stack grows downward on x86/x64, so base > current during normal execution
    return (base > current) ? static_cast<size_t>(base - current)
                            : static_cast<size_t>(current - base);
  }

  size_t limit;
  std::uintptr_t base;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // defined(_MSC_VER)
