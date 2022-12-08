#pragma once

#include <cstdlib>
#include "PlatformUtils.h"

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
  StackCheck() : limit(PlatformUtils::stackLimit()) {
    unsigned char c;
    ptr = &c; // NOLINT(*StackAddressEscape)
  }
  inline unsigned long size() {
    unsigned char c;
    return std::labs(ptr - &c);
  }

  unsigned long limit;
  unsigned char *ptr;
};
