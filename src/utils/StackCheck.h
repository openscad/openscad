#pragma once

#include <cstddef>

#include "platform/PlatformUtils.h"

#if defined(_MSC_VER)
#include <windows.h>
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

  inline bool check()
  {
#if defined(_MSC_VER)
    // On Windows, use the actual stack limits from the OS
    ULONG_PTR lowLimit, highLimit;
    GetCurrentThreadStackLimits(&lowLimit, &highLimit);

    unsigned char c;
    ULONG_PTR currentSP = reinterpret_cast<ULONG_PTR>(&c);

    // Stack grows downward, so remaining space is current - low
    ULONG_PTR remaining = currentSP - lowLimit;

    return remaining <= STACK_BUFFER_SIZE;
#else
    return size() >= limit;
#endif
  }

private:
  StackCheck() : limit(PlatformUtils::stackLimit())
  {
    unsigned char c;
    ptr = &c;  // NOLINT(*StackAddressEscape)
  }

  inline unsigned long size()
  {
    unsigned char c;
    const auto diff = ptr - &c;
    return diff >= 0 ? static_cast<unsigned long>(diff) : static_cast<unsigned long>(-diff);
  }

  unsigned long limit;
  unsigned char *ptr;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // defined(_MSC_VER)
