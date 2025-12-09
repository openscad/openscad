#pragma once


#include <cstddef>
#include <cstdio>

#include "platform/PlatformUtils.h"

#if defined(_MSC_VER)
#include <windows.h>
#ifndef OPENSCAD_MSVC_RECURSION_LIMIT
#define OPENSCAD_MSVC_RECURSION_LIMIT 1000
#endif
#pragma warning(push)
#pragma warning(disable : 26486)  // Disable warning for dangling pointers
#endif  // defined(_MSC_VER)

// Debug output control - set to 1 to enable verbose stack checking output
#define STACKCHECK_DEBUG 0

#if STACKCHECK_DEBUG
#define STACKCHECK_LOG(...) do { fprintf(stderr, "[StackCheck] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } while(0)
#else
#define STACKCHECK_LOG(...) do {} while(0)
#endif

class StackCheck
{
public:
  static StackCheck& inst()
  {
    static StackCheck instance;
    return instance;
  }

#if defined(_MSC_VER)
  // Structure to hold stack info for debugging
  struct StackInfo {
    ULONG_PTR lowLimit;
    ULONG_PTR highLimit;
    ULONG_PTR currentSP;
    ULONG_PTR totalSize;
    ULONG_PTR used;
    ULONG_PTR remaining;
    ULONG_PTR bufferSize;
    bool limitTriggered;
  };

  inline StackInfo getStackInfo() {
    StackInfo info;
    GetCurrentThreadStackLimits(&info.lowLimit, &info.highLimit);
    
    unsigned char c;
    info.currentSP = reinterpret_cast<ULONG_PTR>(&c);
    info.totalSize = info.highLimit - info.lowLimit;
    info.used = info.highLimit - info.currentSP;
    info.remaining = info.currentSP - info.lowLimit;
    info.bufferSize = STACK_BUFFER_SIZE;
    info.limitTriggered = info.remaining <= STACK_BUFFER_SIZE;
    
    return info;
  }
  
  inline void logStackInfo(const char* context, size_t depth = 0) {
    StackInfo info = getStackInfo();
    STACKCHECK_LOG("%s: depth=%zu, total=%lluKB, used=%lluKB (%.1f%%), remaining=%lluKB, buffer=%lluKB, triggered=%s",
      context,
      depth,
      (unsigned long long)(info.totalSize / 1024),
      (unsigned long long)(info.used / 1024),
      (double)info.used * 100.0 / info.totalSize,
      (unsigned long long)(info.remaining / 1024),
      (unsigned long long)(info.bufferSize / 1024),
      info.limitTriggered ? "YES" : "no");
  }
#endif

  inline bool check() { 
#if defined(_MSC_VER)
    // On Windows, use the actual stack limits from the OS
    ULONG_PTR lowLimit, highLimit;
    GetCurrentThreadStackLimits(&lowLimit, &highLimit);
    
    // Get current stack pointer
    unsigned char c;
    ULONG_PTR currentSP = reinterpret_cast<ULONG_PTR>(&c);
    
    // Stack grows downward, so remaining space is current - low
    // We want to trigger when we're within STACK_BUFFER_SIZE of the limit
    ULONG_PTR remaining = currentSP - lowLimit;
    
    bool triggered = remaining <= STACK_BUFFER_SIZE;
    
#if STACKCHECK_DEBUG
    static thread_local size_t check_count = 0;
    static thread_local size_t last_logged_depth = 0;
    check_count++;
    
    // Log every 100 checks or when close to limit or when triggered
    ULONG_PTR totalSize = highLimit - lowLimit;
    ULONG_PTR used = highLimit - currentSP;
    double usedPercent = (double)used * 100.0 / totalSize;
    
    if (triggered || usedPercent > 90.0 || (check_count % 500 == 0)) {
      STACKCHECK_LOG("check() #%zu: total=%lluKB, used=%lluKB (%.1f%%), remaining=%lluKB, buffer=%lluKB, TRIGGERED=%s",
        check_count,
        (unsigned long long)(totalSize / 1024),
        (unsigned long long)(used / 1024),
        usedPercent,
        (unsigned long long)(remaining / 1024),
        (unsigned long long)(STACK_BUFFER_SIZE / 1024),
        triggered ? "YES!" : "no");
    }
#endif
    
    return triggered;
#else
    return size() >= limit; 
#endif
  }

  class RecursionLimitGuard
  {
  public:
    RecursionLimitGuard() = default;

    RecursionLimitGuard(const RecursionLimitGuard&) = delete;
    RecursionLimitGuard& operator=(const RecursionLimitGuard&) = delete;

    inline bool limitReached()
    {
#if defined(_MSC_VER)
      if (limit_reached) {
        return true;
      }
      if (recursion_depth >= OPENSCAD_MSVC_RECURSION_LIMIT) {
        limit_reached = true;
        STACKCHECK_LOG("RecursionLimitGuard: LIMIT REACHED at depth %zu (limit=%d)", 
          recursion_depth, OPENSCAD_MSVC_RECURSION_LIMIT);
        StackCheck::inst().logStackInfo("RecursionLimit", recursion_depth);
        return true;
      }
      ++recursion_depth;
      
#if STACKCHECK_DEBUG
      // Log at certain depths to track progress - MORE FREQUENT now
      if (recursion_depth <= 5 || recursion_depth % 50 == 0) {
        StackCheck::inst().logStackInfo("RecursionGuard++", recursion_depth);
      }
#endif
#endif
      return false;
    }

    ~RecursionLimitGuard()
    {
#if defined(_MSC_VER)
      if (!limit_reached && recursion_depth > 0) {
#if STACKCHECK_DEBUG
        // Log unwinding at certain depths - MORE FREQUENT now
        if (recursion_depth <= 5 || recursion_depth % 50 == 0) {
          StackCheck::inst().logStackInfo("RecursionGuard--", recursion_depth);
        }
#endif
        --recursion_depth;
      }
#endif
    }
    
#if defined(_MSC_VER)
    static size_t getCurrentDepth() { return recursion_depth; }
#else
    static size_t getCurrentDepth() { return 0; }
#endif

  private:
#if defined(_MSC_VER)
    static inline thread_local size_t recursion_depth = 0;
    bool limit_reached = false;
#endif
  };

private:
  StackCheck() : limit(PlatformUtils::stackLimit())
  {
    unsigned char c;
    ptr = &c;  // NOLINT(*StackAddressEscape)
    
#if defined(_MSC_VER) && STACKCHECK_DEBUG
    ULONG_PTR lowLimit, highLimit;
    GetCurrentThreadStackLimits(&lowLimit, &highLimit);
    STACKCHECK_LOG("INIT: lowLimit=0x%llx, highLimit=0x%llx, total=%lluKB, buffer=%lluKB",
      (unsigned long long)lowLimit,
      (unsigned long long)highLimit, 
      (unsigned long long)((highLimit - lowLimit) / 1024),
      (unsigned long long)(STACK_BUFFER_SIZE / 1024));
    STACKCHECK_LOG("INIT: OPENSCAD_MSVC_RECURSION_LIMIT=%d", OPENSCAD_MSVC_RECURSION_LIMIT);
#endif
  }

  inline unsigned long size()
  {
    unsigned char c;
    const auto diff = ptr - &c;
    return diff >= 0 ? static_cast<unsigned long>(diff)
                     : static_cast<unsigned long>(-diff);
  }

  unsigned long limit;
  unsigned char *ptr;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  // defined(_MSC_VER)
