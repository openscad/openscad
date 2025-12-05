#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstdio>  // DEBUG: for fprintf
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

  inline bool check() {
    const size_t currentSize = size();
    // DEBUG: Print stack usage periodically (every ~1MB of growth)
    static size_t lastReportedMB = 0;
    const size_t currentMB = currentSize / (1024 * 1024);
    if (currentMB > lastReportedMB) {
      lastReportedMB = currentMB;
      std::fprintf(stderr, "DEBUG StackCheck: used=%zu bytes (%.2f MB), limit=%zu bytes (%.2f MB), remaining=%zu bytes\n",
                   currentSize, currentSize / (1024.0 * 1024.0),
                   limit, limit / (1024.0 * 1024.0),
                   (limit > currentSize) ? (limit - currentSize) : 0);
    }
    return currentSize >= limit;
  }

private:
  StackCheck() : limit(PlatformUtils::stackLimit())
  {
    unsigned char c;
    base = reinterpret_cast<std::uintptr_t>(&c);  // NOLINT(*reinterpret-cast, *StackAddressEscape)
    // DEBUG: Print initial stack configuration
    std::fprintf(stderr, "DEBUG StackCheck initialized: base=%p, limit=%zu bytes (%.2f MB), STACK_BUFFER_SIZE=%zu bytes\n",
                 reinterpret_cast<void*>(base), limit, limit / (1024.0 * 1024.0), static_cast<size_t>(STACK_BUFFER_SIZE));
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
