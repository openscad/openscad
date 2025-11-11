/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2025 Clifford Wolf <clifford@clifford.at> and
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

#include "gui/OpenAiApi.h"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QIODevice>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include "core/Settings.h"
#include "utils/printutils.h"
#include "gui/Network.h"

const QString OpenAiApi::url() const
{
  return QString::fromStdString(Settings::Settings::aiApiUrl.value());
}

const QString OpenAiApi::apiKey() const
{
  return QString::fromStdString(Settings::Settings::aiApiKey.value());
}

const QJsonDocument OpenAiApi::getJsonData(const QString& endpoint) const
{
  if (url().trimmed().isEmpty()) {
    throw NetworkException{QNetworkReply::ProtocolFailure, "OpenAiApi URL not configured."};
  }

  auto networkRequest = NetworkRequest<const QJsonDocument>{QUrl{url() + endpoint}, {200}, 30};

  return networkRequest.execute(
    [&](QNetworkRequest& request) {
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
      if (!apiKey().isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + apiKey().toUtf8());
      }
    },
    [](QNetworkAccessManager& nam, QNetworkRequest& request) { return nam.get(request); },
    [](QNetworkReply *reply) -> const QJsonDocument {
      auto doc = QJsonDocument::fromJson(reply->readAll());
      PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
      return doc;
    });
}

const QList<QPair<QString, QString>> OpenAiApi::getModels() const
{
  const auto obj = getJsonData("/models").object();
  QList<QPair<QString, QString>> models;
  const auto& data = obj.value("data").toArray();
  for (const auto& entry : data) {
    const auto& id = entry.toObject().value("id").toString();
    models.append(QPair<QString, QString>(id, id));
  }
  std::sort(models.begin(), models.end());
  return models;
}
