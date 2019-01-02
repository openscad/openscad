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

#include <tuple>
#include <QObject>
#include <QtNetwork>
#include <QJsonDocument>

#include "settings.h"
#include "OctoPrint.h"
#include "printutils.h"
#include "PlatformUtils.h"

OctoPrint::OctoPrint()
{
	user_agent = PlatformUtils::user_agent();
}

OctoPrint::~OctoPrint()
{
}

const std::string OctoPrint::url() const
{
	return Settings::Settings::inst()->get(Settings::Settings::octoPrintUrl).toString();
}

const std::string OctoPrint::api_key() const
{
	return Settings::Settings::inst()->get(Settings::Settings::octoPrintApiKey).toString();
}

const QJsonDocument OctoPrint::get_json_data(const std::string endpoint) const
{
	QNetworkRequest request(QUrl(QString::fromStdString(url() + endpoint)));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(user_agent));
	request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});

	QNetworkAccessManager nam;
	QNetworkReply *reply = nam.get(request);

	QTimer timer;
	timer.setSingleShot(true);

	QEventLoop loop;
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(10000);
	loop.exec();

	reply->deleteLater();
	const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	PRINTB("Status code (endpoint = %s): %3d", endpoint % statusCode.toInt());
	const auto doc = QJsonDocument::fromJson(reply->readAll());
    PRINTDB("Received this JSON in response: %s", QString{doc.toJson()}.toStdString());
	return doc;
}

const std::vector<std::pair<std::string, std::string>> OctoPrint::get_slicers() const
{
	const auto doc = get_json_data("/slicing");
	const auto obj = doc.object();
	
	std::vector<std::pair<std::string, std::string>> slicers;
	for (const auto & key : obj.keys()) {
		slicers.emplace_back(std::pair<std::string, std::string>{key.toStdString(), obj[key].toObject().value("displayName").toString().toStdString()});
	}
	return slicers;
}

const std::vector<std::pair<std::string, std::string>> OctoPrint::get_profiles(const QString slicer) const
{
	const auto doc = get_json_data("/slicing");
	const auto obj = doc.object();

	std::vector<std::pair<std::string, std::string>> profiles;
	for (const auto & key : obj.keys()) {
		const auto entry = obj[key].toObject();
		const auto name = entry.value("key").toString();
		const auto isDefault = entry.value("default").toBool();
		if ((slicer == name) || (slicer.isEmpty() && isDefault)) {
			const auto profilesObject = entry.value("profiles").toObject();
			for (const auto & profileKey : profilesObject.keys()) {
				const auto displayName = profilesObject[profileKey].toObject().value("displayName").toString();
				profiles.emplace_back(std::pair<std::string, std::string>{profileKey.toStdString(), displayName.toStdString()});
			}
			break;
		}
	}
	return profiles;
}

const std::tuple<std::string, std::string, std::string> OctoPrint::get_version() const
{
	const auto doc = get_json_data("/version");

	const auto jsonOutput = doc.object();
    const auto api_version = jsonOutput.value("api").toString().toStdString();
    const auto server_version = jsonOutput.value("server").toString().toStdString();
	const auto result = std::make_tuple("", api_version, server_version);
	return result;
}

const QString OctoPrint::upload(QFile *file, const QString fileName) const {

	QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant{"form-data; name=\"file\"; filename=\"" + fileName + "\""});
	filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant{"application/octet-stream"});

    file->open(QIODevice::ReadOnly);
    file->setParent(multiPart);
    filePart.setBodyDevice(file);

    multiPart->append(filePart);

	QNetworkRequest request(QUrl(QString::fromStdString(url() + "/files/local")));
	request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(user_agent));
	request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});

	QNetworkAccessManager nam;
	QNetworkReply *reply = nam.post(request, multiPart);
    multiPart->setParent(reply);

	QTimer timer;
	timer.setSingleShot(true);

	QEventLoop loop;
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(10 * 60 * 1000);
	loop.exec();

	reply->deleteLater();
	const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	PRINTB("Status code: %3d", statusCode.toInt());
	PRINTB("Result: %s", reply->readAll().toStdString());
	const auto location = reply->header(QNetworkRequest::LocationHeader).toString();
	PRINTB("Location: %s", location.toStdString());
	return location;
}

void OctoPrint::slice(const QString url, const QString slicer, const QString profile, const bool select, const bool print) const
{
	auto request = QNetworkRequest{QUrl{url}};
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(user_agent));
	request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});

	QJsonObject jsonInput;
	jsonInput.insert("command", "slice");
	jsonInput.insert("slicer", slicer);
	jsonInput.insert("profile", profile);
	jsonInput.insert("select", select ? "true" : "false");
	jsonInput.insert("print", print ? "true" : "false");

	QNetworkAccessManager nam;
	QNetworkReply *reply = nam.post(request, QJsonDocument(jsonInput).toJson());

	QTimer timer;
	timer.setSingleShot(true);

	QEventLoop loop;
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(10000);
	loop.exec();

	reply->deleteLater();
	const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	PRINTB("Status code (slice): %3d", statusCode.toInt());
	const auto doc = QJsonDocument::fromJson(reply->readAll());
    PRINTB("Received this JSON in response: %s", QString{doc.toJson()}.toStdString());
}
