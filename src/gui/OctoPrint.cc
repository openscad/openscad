/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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

#include "gui/OctoPrint.h"

#include "gui/Settings.h"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QIODevice>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <string>
#include <utility>
#include <vector>

#include "utils/printutils.h"
#include "platform/PlatformUtils.h"

const QString OctoPrint::url() const
{
  return QString::fromStdString(Settings::Settings::octoPrintUrl.value());
}

const std::string OctoPrint::apiKey() const
{
  return Settings::Settings::octoPrintApiKey.value();
}

const QJsonDocument OctoPrint::getJsonData(const QString& endpoint) const
{
  if (url().trimmed().isEmpty()) {
    throw NetworkException{QNetworkReply::ProtocolFailure, "OctoPrint URL not configured."};
  }

  auto networkRequest = NetworkRequest<const QJsonDocument>{QUrl{url() + endpoint}, { 200 }, 30};

  return networkRequest.execute(
    [&](QNetworkRequest& request) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{apiKey().c_str()});
  },
    [](QNetworkAccessManager& nam, QNetworkRequest& request) {
    return nam.get(request);
  },
    [](QNetworkReply *reply) -> const QJsonDocument {
    auto doc = QJsonDocument::fromJson(reply->readAll());
    PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
    return doc;
  }
  );
}

const std::vector<std::pair<const QString, const QString>> OctoPrint::getSlicers() const
{
  const auto obj = getJsonData("/slicing").object();
  std::vector<std::pair<const QString, const QString>> slicers;
  for (const auto& key : obj.keys()) {
    slicers.emplace_back(std::make_pair(key, obj[key].toObject().value("displayName").toString()));
  }
  return slicers;
}

const std::vector<std::pair<const QString, const QString>> OctoPrint::getProfiles(const QString& slicer) const
{
  const auto obj = getJsonData("/slicing").object();
  std::vector<std::pair<const QString, const QString>> profiles;
  for (const auto& key : obj.keys()) {
    const auto entry = obj[key].toObject();
    const auto name = entry.value("key").toString();
    const auto isDefault = entry.value("default").toBool();
    if ((slicer == name) || (slicer.isEmpty() && isDefault)) {
      const auto profilesObject = entry.value("profiles").toObject();
      for (const auto& profileKey : profilesObject.keys()) {
        const auto displayName = profilesObject[profileKey].toObject().value("displayName").toString();
        profiles.emplace_back(std::make_pair(profileKey, displayName));
      }
      break;
    }
  }
  return profiles;
}

const std::pair<const QString, const QString> OctoPrint::getVersion() const
{
  const auto obj = getJsonData("/version").object();
  const auto api_version = obj.value("api").toString();
  const auto server_version = obj.value("server").toString();
  const auto result = std::make_pair(api_version, server_version);
  return result;
}

const QString OctoPrint::upload(const QString& exportFileName, const QString& fileName, const network_progress_func_t& progress_func) const {

  auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
  QHttpPart filePart;
  filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant{R"(form-data; name="file"; filename=")" + fileName + "\""});
  filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant{"application/octet-stream"});

  auto *file = new QFile(exportFileName, multiPart);
  file->open(QIODevice::ReadOnly);
  filePart.setBodyDevice(file);

  multiPart->append(filePart);

  auto networkRequest = NetworkRequest<const QString>{QUrl{url() + "/files/local"}, { 200, 201 }, 180};
  networkRequest.set_progress_func(progress_func);
  return networkRequest.execute(
    [&](QNetworkRequest& request) {
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(PlatformUtils::user_agent()));
    request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{apiKey().c_str()});
  },
    [&](QNetworkAccessManager& nam, QNetworkRequest& request) {
    const auto reply = nam.post(request, multiPart);
    multiPart->setParent(reply);
    return reply;
  },
    [](QNetworkReply *reply) -> const QString {
    const auto doc = QJsonDocument::fromJson(reply->readAll());
    PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
    auto location = reply->header(QNetworkRequest::LocationHeader).toString();
    LOG("Uploaded successfully to %1$s", location.toStdString());
    return location;
  }
    );
}

void OctoPrint::slice(const QString& fileUrl, const QString& slicer, const QString& profile, const bool select, const bool print) const
{
  QJsonObject jsonInput;
  jsonInput.insert("command", QString{"slice"});
  jsonInput.insert("slicer", slicer);
  jsonInput.insert("profile", profile);
  jsonInput.insert("select", QString{select ? "true" : "false"});
  jsonInput.insert("print", QString{print ? "true" : "false"});

  auto networkRequest = NetworkRequest<void>{QUrl{fileUrl}, { 200, 202 }, 30};
  return networkRequest.execute(
    [&](QNetworkRequest& request) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{apiKey().c_str()});
  },
    [&](QNetworkAccessManager& nam, QNetworkRequest& request) {
    return nam.post(request, QJsonDocument(jsonInput).toJson());
  },
    [](QNetworkReply *reply) {
    const auto doc = QJsonDocument::fromJson(reply->readAll());
    PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
    LOG("Slice command successfully executed.");
  }
    );
}
