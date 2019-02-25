/*
 * Find Windows version using bisection method and VerifyVersionInfo.
 *
 * Author:   M1xA, www.m1xa.com
 * Date:     2013.07.07
 * Licence:  MIT
 * Version:  1.0
 *
 * API:
 *
 * BOOL GetVersionExEx(OSVERSIONINFOEX * osVer);
 * Returns: 0 if fails.
 *
 * Supported OS: Windows 2000 .. Windows 8.1.
 */
#ifndef __FIND_VERSION__
#define __FIND_VERSION__

#include <windows.h>

#define FV_EQUAL 0
#define FV_LESS -1
#define FV_GREAT 1
#define FV_MIN_VALUE 0
#define FV_MINOR_VERSION_MAX_VALUE 16

int testValue(OSVERSIONINFOEX *value, DWORD verPart, DWORDLONG eq, DWORDLONG gt)
{
  if (VerifyVersionInfo(value, verPart, eq) == FALSE) {
    if (VerifyVersionInfo(value, verPart, gt) == TRUE) return FV_GREAT;
    return FV_LESS;
  }
  else return FV_EQUAL;
}

DWORDLONG gtFor(DWORD target)
{
  return VerSetConditionMask(0, target, VER_GREATER);
}

DWORDLONG eqFor(DWORD target)
{
  return VerSetConditionMask(0, target, VER_EQUAL);
}

#define findPartTemplate(T) \
  BOOL findPart ## T(T * part, DWORD partType, OSVERSIONINFOEX * ret, T a, T b) \
  { \
    int funx = FV_EQUAL; \
\
    DWORDLONG const eq = eqFor(partType); \
    DWORDLONG const gt = gtFor(partType); \
\
    T *p = part; \
\
    *p = (a + b) / 2; \
\
    while ((funx = testValue(ret, partType, eq, gt)) != FV_EQUAL) \
    { \
      switch (funx) \
      { \
      case FV_GREAT: a = *p; break; \
      case FV_LESS:  b = *p; break; \
      } \
\
      *p = (a + b) / 2; \
\
      if (*p == a) \
      { \
        if (testValue(ret, partType, eq, gt) == FV_EQUAL) return TRUE; \
\
        *p = b; \
\
        if (testValue(ret, partType, eq, gt) == FV_EQUAL) return TRUE; \
\
        a = 0; \
        b = 0; \
        *p = 0; \
      } \
\
      if (a == b) \
      { \
        *p = 0; \
        return FALSE; \
      } \
    } \
\
    return TRUE; \
  }

findPartTemplate(DWORD)
findPartTemplate(WORD)
findPartTemplate(BYTE)

BOOL GetVersionExEx(OSVERSIONINFOEX *osVer)
{
  BOOL ret = TRUE;

  ZeroMemory(osVer, sizeof(OSVERSIONINFOEX));

  osVer->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

  ret &= findPartDWORD(&osVer->dwPlatformId, VER_PLATFORMID, osVer, FV_MIN_VALUE, MAXDWORD);
  ret &= findPartDWORD(&osVer->dwMajorVersion, VER_MAJORVERSION, osVer, FV_MIN_VALUE, MAXDWORD);
  ret &= findPartDWORD(&osVer->dwMinorVersion, VER_MINORVERSION, osVer, FV_MIN_VALUE, FV_MINOR_VERSION_MAX_VALUE);
  ret &= findPartDWORD(&osVer->dwBuildNumber, VER_BUILDNUMBER, osVer, FV_MIN_VALUE, MAXDWORD);
  ret &= findPartWORD(&osVer->wServicePackMajor, VER_SERVICEPACKMAJOR, osVer, FV_MIN_VALUE, MAXWORD);
  ret &= findPartWORD(&osVer->wServicePackMinor, VER_SERVICEPACKMINOR, osVer, FV_MIN_VALUE, MAXWORD);
  ret &= findPartWORD(&osVer->wSuiteMask, VER_SUITENAME, osVer, FV_MIN_VALUE, MAXWORD);
  ret &= findPartBYTE(&osVer->wProductType, VER_PRODUCT_TYPE, osVer, FV_MIN_VALUE, MAXBYTE);

  return ret;
}

#endif // ifndef __FIND_VERSION__
