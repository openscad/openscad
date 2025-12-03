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

  inline unsigned long size() const
  {
    unsigned char c;
    const auto current = reinterpret_cast<std::uintptr_t>(&c);
    return current >= base ? static_cast<unsigned long>(current - base)
                           : static_cast<unsigned long>(base - current);
  }

  unsigned long limit;
  std::uintptr_t base;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // defined(_MSC_VER)
