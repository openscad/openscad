#pragma once

#include "LanguageRuntime.h"

/*!
   Python Language Runtime class.
 */
class PythonLanguageRuntime: public LanguageRuntime
{
public:
  const char* getId() { return "python"; }
  const char* getName() { return "Python"; }
  const char* getDescription() { return "Python Language"; }
  bool isExperimental() { return false; }
  bool isTrusted() { return true; }
  const char* getTrustFlag() { return "trust-python"; }
  const char* getCommentString() { return "#"; }
  const char* getFileExtension() { return ".py"; }
  const char* getFileFilter() { return "Python Files (*.py)"; }
  bool evaluate() { return true; }
};