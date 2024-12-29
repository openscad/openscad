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

#include "gui/PrintService.h"

#include <memory>
#include <string>

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>

#include "export.h"
#include "utils/printutils.h"

std::mutex PrintService::printServiceMutex;

namespace {

std::unique_ptr<PrintService>
createPrintService(const QJsonObject &serviceObject) {
  auto service = std::make_unique<PrintService>();
  if (service->init(serviceObject))
    return service;
  return nullptr;
}

} // namespace

std::unordered_map<std::string, std::unique_ptr<PrintService>>
createPrintServices() {
  std::unordered_map<std::string, std::unique_ptr<PrintService>> printServices;
  // TODO: Where to call this, will we need a mutex?
  try {
    auto networkRequest = NetworkRequest<void>{
        QUrl{"https://app.openscad.org/print-service.json"}, {200}, 30};
    networkRequest.execute(
        [](QNetworkRequest &request) {
          request.setHeader(QNetworkRequest::ContentTypeHeader,
                            "application/json");
        },
        [](QNetworkAccessManager &nam, QNetworkRequest &request) {
          return nam.get(request);
        },
        [&](QNetworkReply *reply) {
          const auto doc = QJsonDocument::fromJson(reply->readAll());
          PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
          const QStringList services = doc.object().keys();
          for (const auto &serviceName : services) {
            auto printService =
                createPrintService(doc.object().value(serviceName).toObject());
            if (printService) {
              printServices[serviceName.toStdString()] =
                  std::move(printService);
            }
          }
        });
  } catch (const NetworkException &e) {
    LOG(message_group::Error, "%1$s", e.getErrorMessage());
  }
  // TODO: Log if wanted
  //    LOG("External print service available: %1$s (upload limit = %2$d MB)",
  //    displayName.toStdString(), fileSizeLimitMB);

  return std::move(printServices);
}

const std::unordered_map<std::string, std::unique_ptr<PrintService>> &
PrintService::getPrintServices() {
  const static std::unordered_map<std::string, std::unique_ptr<PrintService>>
      printServices = createPrintServices();
  return printServices;
}

const PrintService *PrintService::getPrintService(const std::string &name) {
  const auto &printServices = getPrintServices();
  if (const auto it = printServices.find(name); it != printServices.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool PrintService::init(const QJsonObject &serviceObject) {
  displayName = serviceObject.value("displayName").toString();
  apiUrl = serviceObject.value("apiUrl").toString();
  fileSizeLimitMB = serviceObject.value("fileSizeLimitMB").toInt();
  infoHtml = serviceObject.value("infoHtml").toString();
  infoUrl = serviceObject.value("infoUrl").toString();
  for (const auto& variant : serviceObject.value("fileFormats").toArray().toVariantList()) {
    FileFormat fileFormat;
    if (fileformat::fromIdentifier(variant.toString().toStdString(), fileFormat)) {
      fileFormats.push_back(fileFormat);
    } 
    // TODO: else print warning?
  }
  // For legacy reasons; default to STL
  if (fileFormats.empty()) {
    fileFormats.push_back(FileFormat::ASCII_STL);
    fileFormats.push_back(FileFormat::BINARY_STL);
  }
  return !displayName.isEmpty() && !apiUrl.isEmpty() && !infoHtml.isEmpty() &&
         !infoUrl.isEmpty() && fileSizeLimitMB != 0;
}

/**
 * This function uploads a design to the 3D printing API endpoint and
 * returns an URL that, when accessed, will show the design as a part
 * that can be configured and added to the shopping cart. If it's not
 * successful, it throws an exception with a message.
 *
 * Inputs:
 *   fileName - Then name we should give the file when it is uploaded
 *              for the order process.
 * Outputs:
 *    The resulting url to go to next to continue the order process.
 */
const QString
PrintService::upload(const QString &fileName, const QString &contentBase64,
                     const network_progress_func_t &progress_func) const {
  QJsonObject jsonInput;
  jsonInput.insert("fileName", fileName);
  jsonInput.insert("file", contentBase64);

  // Safe guard against QJson silently dropping the file content if it's
  // too big. This seems to be configured at MaxSize = (1<<27) - 1 in Qt
  // via qtbase/src/corelib/json/qjson_p.h
  // Due to the base64 encoding having 33% overhead, that should allow for
  // about 96MB data.
  if (jsonInput.value("file") == QJsonValue::Undefined) {
    const QString msg =
        "Could not encode STL into JSON. Perhaps it is too large of a file? "
        "Maybe try reducing the model resolution.";
    throw NetworkException(QNetworkReply::ProtocolFailure, msg);
  }

  auto networkRequest =
      NetworkRequest<const QString>{QUrl{apiUrl}, {200, 201}, 180};
  networkRequest.set_progress_func(progress_func);
  return networkRequest.execute(
      [](QNetworkRequest &request) {
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          "application/json");
      },
      [&](QNetworkAccessManager &nam, QNetworkRequest &request) {
        return nam.post(request, QJsonDocument(jsonInput).toJson());
      },
      [](QNetworkReply *reply) {
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());

        // Extract cartUrl which gives the page to open in a webbrowser to view
        // uploaded part
        auto cartUrlValue =
            doc.object().value("data").toObject().value("cartUrl");
        auto cartUrl = cartUrlValue.toString();
        if ((cartUrlValue == QJsonValue::Undefined) || (cartUrl.isEmpty())) {
          const QString msg = "Could not get data.cartUrl field from response.";
          throw NetworkException(QNetworkReply::ProtocolFailure, msg);
        }
        LOG("Upload finished, opening URL %1$s.", cartUrl.toStdString());
        return cartUrl;
      });
}
