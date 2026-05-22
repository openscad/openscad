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
bool EditorInterface::hasPythonTrustHash(void) const
{
  if (filepath.isEmpty()) return false;
  const QByteArray pathBytes = filepath.toUtf8();
  const std::string pathUtf8(pathBytes.constData(), static_cast<size_t>(pathBytes.size()));
  QSettingsCached settings;
  // Pure presence check — no migration side-effects. Check all key variants.
  return settings.contains(pythonTrustSettingKeyNew(pathUtf8)) ||
         settings.contains(pythonTrustSettingKeyLegacyLocal(pathUtf8)) ||
         settings.contains(pythonTrustSettingKeyLegacyCharCtorUtf8(pathUtf8)) ||
         settings.contains(pythonTrustSettingKeyLegacyRawUtf8(pathUtf8));
}

bool EditorInterface::trust_python_file(void)
{
  if (python_trusted || Settings::SettingsPython::globalTrustPython.value() || filepath.isEmpty()) {
    // Global/CLI trust or unsaved buffer: effective trust without a per-file hash.
    // Do NOT set trusted=true here — trusted is reserved for hash-verified per-file trust.
    // Callers check effective trust as: trusted || python_trusted || globalTrustPython.
    return true;
  }

  if (trusted) return true;

  const QByteArray contentBytes = toPlainText().toUtf8();
  const std::string act_hash =
    SHA256HashString(std::string(contentBytes.constData(), static_cast<size_t>(contentBytes.size())));

  QSettingsCached settings;
  const std::string ref_hash =
    readPythonTrustHash(settings, filepath.toUtf8().constData()).toStdString();

  if (act_hash == ref_hash) {
    trusted = true;
    emit trustStateChanged();
    return true;
  }

  trusted = false;
  emit trustStateChanged();
  return false;
}

void EditorInterface::trustCurrent(void)
{
#ifdef ENABLE_PYTHON
  if (language != LANG_PYTHON) {
    QMessageBox::information(this, _("Python"), _("The active design is not a Python design."));
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
  writePythonTrustHash(settings, fpath, SHA256HashString(content));
  trusted = true;
  emit trustStateChanged();
#endif
}
void EditorInterface::revokeTrust(void)
{
  trusted = false;
  emit trustStateChanged();
}

#endif
