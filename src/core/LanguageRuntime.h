#pragma once

/*!
   Language Runtime is an abstract class for new language support.
 */
class LanguageRuntime
{
public:
  virtual const char* getId() = 0;
  virtual const char* getName() = 0;
  virtual const char* getDescription() = 0;
  virtual bool isExperimental() = 0;
  virtual bool isTrusted() = 0;
  virtual const char* getTrustFlag() = 0;
  virtual const char* getCommentString() = 0;
  virtual const char* getFileExtension() = 0;
  virtual const char* getFileFilter() = 0;
  virtual bool evaluate() = 0;
};
