#include "LanguageRuntime.h"

const char* LanguageRuntime::getFileSuffix()
{
  auto suffix = getFileExtension();
  return ++suffix;
}