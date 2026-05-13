#include "gui/Editor.h"

#include <QWidget>

#include "gui/Preferences.h"
#include "gui/QSettingsCached.h"
#include <QMessageBox>

#ifdef ENABLE_PYTHON
#include "nettle/base64.h"
#include "nettle/sha2.h"
#include "nettle/version.h"
#include "python/python_public.h"

std::string SHA256HashString(std::string aString)
{
  uint8_t digest[SHA256_DIGEST_SIZE];
  sha256_ctx sha256_ctx;

  sha256_init(&sha256_ctx);
  sha256_update(&sha256_ctx, aString.length(), (uint8_t *)aString.c_str());
#if NETTLE_VERSION_MAJOR >= 4
  sha256_digest(&sha256_ctx, digest);
#else
  sha256_digest(&sha256_ctx, SHA256_DIGEST_SIZE, digest);
#endif

  base64_encode_ctx base64_ctx;
  char digest_base64[BASE64_ENCODE_LENGTH(SHA256_DIGEST_SIZE) + 1];
  memset(digest_base64, 0, sizeof(digest_base64));

  base64_encode_init(&base64_ctx);
  base64_encode_update(&base64_ctx, digest_base64, SHA256_DIGEST_SIZE, digest);
  base64_encode_final(&base64_ctx, digest_base64);
  return digest_base64;
}

namespace {

QString pythonTrustSettingKeyNew(const std::string& pathUtf8)
{
  const QByteArray pathUtf8Bytes(pathUtf8.data(), static_cast<int>(pathUtf8.size()));
  return QStringLiteral("python_hash/%1").arg(QString::fromUtf8(pathUtf8Bytes.toPercentEncoding()));
}

// Legacy (pre percent-encoded key): "python_hash/<local8-bit path>" (trust file id from toLocal8Bit()).
QString pythonTrustSettingKeyLegacyLocal(const std::string& pathUtf8)
{
  const QString qpath =
    QString::fromUtf8(QByteArray(pathUtf8.data(), static_cast<int>(pathUtf8.size())));
  return QStringLiteral("python_hash/") + QString::fromLocal8Bit(qpath.toLocal8Bit());
}

// Legacy: key suffix was local 8-bit bytes from snprintf, but some builds passed char[] into
// setValue(QString) via QString(const char*), which decodes as UTF-8 (not fromLocal8Bit).
QString pythonTrustSettingKeyLegacyCharCtorUtf8(const std::string& pathUtf8)
{
  const QString qpath =
    QString::fromUtf8(QByteArray(pathUtf8.data(), static_cast<int>(pathUtf8.size())));
  return QStringLiteral("python_hash/") + QString::fromUtf8(qpath.toLocal8Bit());
}

// Legacy: "python_hash/<raw UTF-8 path>" (brief use of toStdString()/snprintf before UTF-8 key change).
QString pythonTrustSettingKeyLegacyRawUtf8(const std::string& pathUtf8)
{
  return QStringLiteral("python_hash/") +
         QString::fromUtf8(QByteArray(pathUtf8.data(), static_cast<int>(pathUtf8.size())));
}

QString readPythonTrustHash(QSettingsCached& settings, const std::string& pathUtf8)
{
  const QString kNew = pythonTrustSettingKeyNew(pathUtf8);
  if (settings.contains(kNew)) {
    return settings.value(kNew).toString();
  }
  const QString kLegLoc = pythonTrustSettingKeyLegacyLocal(pathUtf8);
  if (settings.contains(kLegLoc)) {
    const QString v = settings.value(kLegLoc).toString();
    settings.setValue(kNew, v);
    settings.remove(kLegLoc);
    return v;
  }
  const QString kLegCharUtf8 = pythonTrustSettingKeyLegacyCharCtorUtf8(pathUtf8);
  if (kLegCharUtf8 != kNew && kLegCharUtf8 != kLegLoc && settings.contains(kLegCharUtf8)) {
    const QString v = settings.value(kLegCharUtf8).toString();
    settings.setValue(kNew, v);
    settings.remove(kLegCharUtf8);
    return v;
  }
  const QString kLegRaw = pythonTrustSettingKeyLegacyRawUtf8(pathUtf8);
  if (kLegRaw != kNew && kLegRaw != kLegLoc && kLegRaw != kLegCharUtf8 && settings.contains(kLegRaw)) {
    const QString v = settings.value(kLegRaw).toString();
    settings.setValue(kNew, v);
    settings.remove(kLegRaw);
    return v;
  }
  return {};
}

void writePythonTrustHash(QSettingsCached& settings, const std::string& pathUtf8,
                          const std::string& hash)
{
  const QString kNew = pythonTrustSettingKeyNew(pathUtf8);
  const QString v = QString::fromStdString(hash);
  settings.setValue(kNew, v);
  settings.remove(pythonTrustSettingKeyLegacyLocal(pathUtf8));
  const QString kLegCharUtf8 = pythonTrustSettingKeyLegacyCharCtorUtf8(pathUtf8);
  if (kLegCharUtf8 != kNew && kLegCharUtf8 != pythonTrustSettingKeyLegacyLocal(pathUtf8)) {
    settings.remove(kLegCharUtf8);
  }
  const QString kLegRaw = pythonTrustSettingKeyLegacyRawUtf8(pathUtf8);
  if (kLegRaw != kNew) {
    settings.remove(kLegRaw);
  }
}
}  // namespace
#endif
void EditorInterface::recomputeLanguageActive()
{
  if (languageManuallySet) return;  // Don't override manual selection

  auto fnameba = filepath.toLocal8Bit();
  const char *fname = filepath.isEmpty() ? "" : fnameba;

  int oldLanguage = language;
  language = LANG_SCAD;
  if (fname != NULL) {
#ifdef ENABLE_PYTHON
    if (boost::algorithm::ends_with(fname, ".py")) {
      language = LANG_PYTHON;
    }
#endif
  }

#ifdef ENABLE_PYTHON
  if (oldLanguage != language) {
    onLanguageChanged(language);
  }
#endif
}

void EditorInterface::setLanguageManually(int lang)
{
  languageManuallySet = true;
  if (language != lang) {
    language = lang;
    onLanguageChanged(lang);
  }
}

void EditorInterface::resetLanguageDetection()
{
  languageManuallySet = false;
  recomputeLanguageActive();
}

#ifdef ENABLE_PYTHON
extern bool python_trusted;
bool EditorInterface::trust_python_file(void)
{
  QSettingsCached settings;
  if (python_trusted) return true;
  if (Settings::SettingsPython::globalTrustPython.value() == true) return true;

  // Trust unsaved files (empty filepath) - they're created by the user, not loaded from disk
  if (filepath.toStdString().empty()) {
    return true;
  }

  std::string act_hash, ref_hash;
  auto content = toPlainText().toUtf8().constData();
  act_hash = SHA256HashString(content);

  if (untrusted) return false;

  if (trusted) {
    writePythonTrustHash(settings, filepath.toUtf8().constData(), act_hash);
    return true;
  }

  if (strlen(content) <= 1) {  // 1st character already typed
    trusted = true;
    return true;
  }
  /*
    // Disabled: PythonSCAD relies on a hash-based trust store (see
    // readPythonTrustHash / writePythonTrustHash) instead of a content
    // sniff.  Kept for historical reference; if anyone re-enables this
    // shortcut, the prefix check must accept all three module names that
    // a PythonSCAD script can legally start with.
    if (content.rfind("from openscad import", 0) == 0 || content.rfind("from pythonscad import", 0) == 0
    || content.rfind("from _openscad import", 0) == 0) { trusted = true; return true;
    }
  */
  ref_hash = readPythonTrustHash(settings, filepath.toUtf8().constData()).toStdString();

  if (act_hash == ref_hash) {
    trusted = true;
    return true;
  }

  auto ret = QMessageBox::warning(this, "Application",
                                  _("Python files can potentially contain harmful stuff.\n"
                                    "Do you trust this file ?\n"),
                                  QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::No);
  if (ret == QMessageBox::YesAll) {
    python_trusted = true;
    return true;
  }
  if (ret == QMessageBox::Yes) {
    trusted = true;
    writePythonTrustHash(settings, filepath.toUtf8().constData(), act_hash);
    return true;
  }

  if (ret == QMessageBox::No) {
    untrusted = true;
    return false;
  }
  return false;
}
void EditorInterface::clearPythonUntrustState(void)
{
  untrusted = false;
}

void EditorInterface::trustCurrent(void)
{
#ifdef ENABLE_PYTHON
  if (language != LANG_PYTHON) {
    QMessageBox::information(this, _("Python"), _("The active document is not a Python file."));
    return;
  }
  if (filepath.isEmpty()) {
    QMessageBox::information(
      this, _("Python"),
      _("Untitled buffers are already trusted. Save to a file if you need a persistent trust entry."));
    return;
  }
  const QByteArray docUtf8 = toPlainText().toUtf8();
  const std::string content(docUtf8.constData(), static_cast<size_t>(docUtf8.size()));
  QSettingsCached settings;
  const QByteArray pathUtf8 = filepath.toUtf8();
  const std::string fpath(pathUtf8.constData(), static_cast<size_t>(pathUtf8.size()));
  clearPythonUntrustState();
  writePythonTrustHash(settings, fpath, SHA256HashString(content));
  trusted = true;
  QMessageBox::information(this, _("Python"), _("This document is now trusted for Python execution."));
#endif
}
void EditorInterface::revokeTrust(void)
{
  trusted = false;
  untrusted = false;
}

#endif
