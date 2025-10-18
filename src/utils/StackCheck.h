#pragma once

#include <cstdlib>
#include "platform/PlatformUtils.h"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif  // defined(__GNUC__) && !defined(__clang__)
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
    ptr = &c;  // NOLINT(*StackAddressEscape)
  }
  inline unsigned long size()
  {
    unsigned char c;
    return std::abs(ptr - &c);
  }

  unsigned long limit;
  unsigned char *ptr;
};
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // defined(__GNUC__) && !defined(__clang__)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // defined(_MSC_VER)
