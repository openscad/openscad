/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2026 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QString>
#include <QSsl>
#include <QSslCertificate>

#include "utils/printutils.h"

/**
 * Load one or more CA certificates from a PEM file.
 *
 * The file may contain a bundle of PEM certificates. Every
 *
 * "-----BEGIN CERTIFICATE-----"
 * ...
 * "-----END CERTIFICATE-----"
 *
 * block is parsed individually via QSslCertificate::fromData().
 *
 * Returns an empty list if the file cannot be opened or read, a warning
 * is logged in that case); unparseable blocks are skipped with a warning.
 */
inline QList<QSslCertificate> loadCustomCaCerts(const QString& path)
{
  // For now, this is reading the file separately, maybe check later
  // if QSslCertificate::fromPath() works better.
  QList<QSslCertificate> certs;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    LOG("Could not open CA certificate file: %1$s", path.toStdString());
    return certs;
  }
  const QByteArray data = file.readAll();

  static const char *beginMarker = "-----BEGIN CERTIFICATE-----";
  static const char *endMarker = "-----END CERTIFICATE-----";
  static const int beginLen = int(strlen(beginMarker));
  static const int endLen = int(strlen(endMarker));

  int pos = 0;
  while (true) {
    const int begin = data.indexOf(beginMarker, pos);
    if (begin < 0) {
      break;
    }
    const int end = data.indexOf(endMarker, begin + beginLen);
    if (end < 0) {
      LOG("Malformed CA certificate file, missing end marker: %1$s", path.toStdString());
      break;
    }
    const int blockEnd = end + endLen;
    const QByteArray block = data.mid(begin, blockEnd - begin);
    for (const QSslCertificate& cert : QSslCertificate::fromData(block, QSsl::Pem)) {
      if (cert.isNull()) {
        LOG("Skipping unparseable certificate in CA certificate file: %1$s", path.toStdString());
      } else {
        certs.append(cert);
      }
    }
    pos = blockEnd;
  }
  if (certs.empty()) {
    LOG("No valid certificates found in CA certificate file: %1$s", path.toStdString());
  }
  return certs;
}
