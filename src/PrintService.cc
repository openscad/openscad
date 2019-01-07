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

#include "Network.h"
#include "PrintService.h"
#include "printutils.h"

std::mutex PrintService::printServiceMutex;

PrintService::PrintService()
{
	enabled = false;
	try {
		init();
	} catch (const NetworkException& e) {
		PRINTB("ERROR: %s", e.getErrorMessage().toStdString());
	}
	if (enabled) {
		PRINTB("External print service available: %s (url = %s, upload limit = %d MB)", displayName.toStdString() % infoUrl.toStdString() % fileSizeLimitMB);
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
	auto networkRequest = NetworkRequest<void>{QUrl{"http://files.openscad.org/print-service.json"}, { 200 }, 30};
	return networkRequest.execute(
			[&](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
			},
			[&](QNetworkAccessManager& nam, QNetworkRequest& request) {
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