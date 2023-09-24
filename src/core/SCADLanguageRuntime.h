#pragma once

#include "LanguageRuntime.h"

/*!
   SCAD Language Runtime class.
 */
class SCADLanguageRuntime: public LanguageRuntime
{
public:
  const char* getId() { return "scad"; }
  const char* getName() { return "SCAD"; }
  const char* getDescription() { return "SCAD Language"; }
  bool isExperimental() { return false; }
  bool isTrusted() { return true; }
  const char* getTrustFlag() { return "trust-scad"; }
  const char* getCommentString() { return "//"; }
  const char* getFileExtension() { return ".scad"; }
  const char* getFileFilter() { return "OpenSCAD Designs (*.scad *.csg)"; }
  bool evaluate() { return true; }
};