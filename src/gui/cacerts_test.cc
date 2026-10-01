#include "gui/CaCerts.h"

#include <catch2/catch_all.hpp>

namespace {

QString testDataPath(const char *name)
{
  return QString(OPENSCAD_TEST_DATA_DIR) + "/certs/" + name;
}

}  // namespace

TEST_CASE("loadCustomCaCerts loads all certificates from a PEM bundle", "[CaCerts]")
{
  const QString path = testDataPath("bundle.pem");

  const QList<QSslCertificate> certs = loadCustomCaCerts(path);

  REQUIRE(certs.size() == 2);
  CHECK(certs[0].subjectDisplayName() == "Test CA One");
  CHECK(certs[1].subjectDisplayName() == "Test CA Two");
}

TEST_CASE("loadCustomCaCerts loads a single certificate", "[CaCerts]")
{
  const QString path = testDataPath("cert-one.pem");

  const QList<QSslCertificate> certs = loadCustomCaCerts(path);

  REQUIRE(certs.size() == 1);
  CHECK(certs[0].subjectDisplayName() == "Test CA One");
}

TEST_CASE("loadCustomCaCerts returns an empty list for a missing file", "[CaCerts]")
{
  const QString path = testDataPath("does-not-exist.pem");

  const QList<QSslCertificate> certs = loadCustomCaCerts(path);

  CHECK(certs.isEmpty());
}

TEST_CASE("loadCustomCaCerts skips unparseable blocks", "[CaCerts]")
{
  const QString path = testDataPath("mixed.pem");

  const QList<QSslCertificate> certs = loadCustomCaCerts(path);

  REQUIRE(certs.size() == 1);
  CHECK(certs[0].subjectDisplayName() == "Test CA One");
}

TEST_CASE("loadCustomCaCerts returns an empty list for a file without certificates", "[CaCerts]")
{
  const QString path = testDataPath("nonsense.pem");

  const QList<QSslCertificate> certs = loadCustomCaCerts(path);

  CHECK(certs.isEmpty());
}
