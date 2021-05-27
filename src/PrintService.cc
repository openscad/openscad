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

#include "printutils.h"
#include "boost-utils.h"
#include "PrintService.h"

std::mutex PrintService::printServiceMutex;

PrintService::PrintService()
{
	enabled = false;
	try {
		init();
	} catch (const NetworkException& e) {
		LOG(message_group::Error,Location::NONE,"","%1$s",e.getErrorMessage());
	}
	if (enabled) {
		LOG(message_group::None,Location::NONE,"","External print service available: %1$s (upload limit = %2$d MB)",displayName.toStdString(),fileSizeLimitMB);
	}
}

PrintService::~PrintService()
{
}

PrintService * PrintService::inst()
{
	static PrintService *instance = nullptr;

	std::lock_guard<std::mutex> lock(printServiceMutex);
	
	if (instance == nullptr) {
		instance = new PrintService();
	}

	return instance;
}

void PrintService::init()
{
	auto networkRequest = NetworkRequest<void>{QUrl{"https://files.openscad.org/print-service.json"}, { 200 }, 30};
	return networkRequest.execute(
			[](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
			},
			[](QNetworkAccessManager& nam, QNetworkRequest& request) {
				return nam.get(request);
			},
			[&](QNetworkReply *reply) {
				const auto doc = QJsonDocument::fromJson(reply->readAll());
				PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
				const QStringList services = doc.object().keys();
				if (services.length() >= 1) {
					service = services.at(0);
					if (!service.isEmpty()) {
						initService(doc.object().value(service).toObject());
					}
				}
			}
	);
}

void PrintService::initService(const QJsonObject& serviceObject)
{
	displayName = serviceObject.value("displayName").toString();
	apiUrl = serviceObject.value("apiUrl").toString();
	fileSizeLimitMB = serviceObject.value("fileSizeLimitMB").toInt();
	infoHtml = serviceObject.value("infoHtml").toString();
	infoUrl = serviceObject.value("infoUrl").toString();
	enabled = !displayName.isEmpty() && !apiUrl.isEmpty() && !infoHtml.isEmpty() && !infoUrl.isEmpty() && fileSizeLimitMB != 0;
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
const QString PrintService::upload(const QString& fileName, const QString& contentBase64, network_progress_func_t progress_func)
{
	QJsonObject jsonInput;
	jsonInput.insert("fileName", fileName);
	jsonInput.insert("file", contentBase64);

    // Safe guard against QJson silently dropping the file content if it's
	// too big. This seems to be configured at MaxSize = (1<<27) - 1 in Qt
	// via qtbase/src/corelib/json/qjson_p.h
	// Due to the base64 encoding having 33% overhead, that should allow for
	// about 96MB data.
    if (jsonInput.value("file") == QJsonValue::Undefined) {
		const QString msg = "Could not encode STL into JSON. Perhaps it is too large of a file? Maybe try reducing the model resolution.";
        throw NetworkException(QNetworkReply::ProtocolFailure, msg);
	}

	auto networkRequest = NetworkRequest<const QString>{QUrl{apiUrl}, { 200, 201 }, 180};
	networkRequest.set_progress_func(progress_func);
	return networkRequest.execute(
			[](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
			},
			[&](QNetworkAccessManager& nam, QNetworkRequest& request) {
				return nam.post(request, QJsonDocument(jsonInput).toJson());
			},
			[](QNetworkReply *reply) {
				const auto doc = QJsonDocument::fromJson(reply->readAll());
				PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());

				// Extract cartUrl which gives the page to open in a webbrowser to view uploaded part
				const auto cartUrlValue = doc.object().value("data").toObject().value("cartUrl");
				const auto cartUrl = cartUrlValue.toString();
				if ((cartUrlValue == QJsonValue::Undefined) || (cartUrl.isEmpty())) {
					const QString msg = "Could not get data.cartUrl field from response.";
					throw NetworkException(QNetworkReply::ProtocolFailure, msg);
				}
				LOG(message_group::None,Location::NONE,"","Upload finished, opening URL %1$s.",cartUrl.toStdString());
				return cartUrl;
			}
	);
}
