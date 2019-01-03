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
#include <QJsonDocument>

#include "settings.h"
#include "OctoPrint.h"
#include "printutils.h"
#include "PlatformUtils.h"

OctoPrint::OctoPrint()
{
}

OctoPrint::~OctoPrint()
{
}

const QString OctoPrint::url() const
{
	return QString::fromStdString(Settings::Settings::inst()->get(Settings::Settings::octoPrintUrl).toString());
}

const std::string OctoPrint::api_key() const
{
	return Settings::Settings::inst()->get(Settings::Settings::octoPrintApiKey).toString();
}

using setup_func_t = std::function<void(QNetworkRequest&)>;
using reply_func_t = std::function<QNetworkReply*(QNetworkAccessManager&, QNetworkRequest&)>;

template <typename ResultType>
ResultType read_sync(const QUrl url, const std::vector<int> accepted_codes, setup_func_t setup_func, reply_func_t reply_func, std::function<ResultType(QNetworkReply *)> transform_func)
{
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(PlatformUtils::user_agent()));
	setup_func(request);

	QNetworkAccessManager nam;
	QNetworkReply *reply = reply_func(nam, request);

	QTimer timer;
	timer.setSingleShot(true);

	QEventLoop loop;
	QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(30000);
	loop.exec();

	bool isTimeout = false;
	if (timer.isActive()) {
		timer.stop();
	}
	if (!reply->isFinished()) {
		reply->abort();
		isTimeout = true;
	}

	reply->deleteLater();
	if (isTimeout) {
		throw NetworkException{QNetworkReply::TimeoutError, _("Timeout error")};
	}
	if (reply->error() != QNetworkReply::NoError) {
		throw NetworkException{reply->error(), reply->errorString()};
	}
	const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (std::find(accepted_codes.begin(), accepted_codes.end(), statusCode) == accepted_codes.end()) {
		QString reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
		throw NetworkException{QNetworkReply::ProtocolFailure, reason};
	}

	return transform_func(reply);
}

const QJsonDocument OctoPrint::get_json_data(const QString endpoint) const
{
	return read_sync<const QJsonDocument>(QUrl(url() + endpoint), { 200 },
			[&](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
				request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});
			},
			[](QNetworkAccessManager& nam, QNetworkRequest& request) {
				return nam.get(request);
			},
			[](QNetworkReply *reply) -> const QJsonDocument {
				const auto doc = QJsonDocument::fromJson(reply->readAll());
				PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
				return doc;
			}
	);
}

const std::vector<std::pair<const QString, const QString>> OctoPrint::get_slicers() const
{
	const auto doc = get_json_data("/slicing");

	const auto obj = doc.object();
	std::vector<std::pair<const QString, const QString>> slicers;
	for (const auto & key : obj.keys()) {
		slicers.emplace_back(std::make_pair(key, obj[key].toObject().value("displayName").toString()));
	}
	return slicers;
}

const std::vector<std::pair<const QString, const QString>> OctoPrint::get_profiles(const QString slicer) const
{
	const auto doc = get_json_data("/slicing");

	const auto obj = doc.object();
	std::vector<std::pair<const QString, const QString>> profiles;
	for (const auto & key : obj.keys()) {
		const auto entry = obj[key].toObject();
		const auto name = entry.value("key").toString();
		const auto isDefault = entry.value("default").toBool();
		if ((slicer == name) || (slicer.isEmpty() && isDefault)) {
			const auto profilesObject = entry.value("profiles").toObject();
			for (const auto & profileKey : profilesObject.keys()) {
				const auto displayName = profilesObject[profileKey].toObject().value("displayName").toString();
				profiles.emplace_back(std::make_pair(profileKey, displayName));
			}
			break;
		}
	}
	return profiles;
}

const std::pair<const QString, const QString> OctoPrint::get_version() const
{
	const auto obj = get_json_data("/version").object();
    const auto api_version = obj.value("api").toString();
    const auto server_version = obj.value("server").toString();
	const auto result = std::make_pair(api_version, server_version);
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

	return read_sync<const QString>(QUrl(url() + "/files/local"), { 200, 201 },
			[&](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(PlatformUtils::user_agent()));
				request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});
			},
			[&](QNetworkAccessManager& nam, QNetworkRequest& request) {
				const auto reply = nam.post(request, multiPart);
			    multiPart->setParent(reply);
				return reply;
			},
			[](QNetworkReply *reply) -> const QString {
				const auto doc = QJsonDocument::fromJson(reply->readAll());
				PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
				const auto location = reply->header(QNetworkRequest::LocationHeader).toString();
				PRINTB("Uploaded successfully to %s", location.toStdString());
				return location;
			}
	);
}

void OctoPrint::slice(const QString url, const QString slicer, const QString profile, const bool select, const bool print) const
{
	QJsonObject jsonInput;
	jsonInput.insert("command", QString{"slice"});
	jsonInput.insert("slicer", slicer);
	jsonInput.insert("profile", profile);
	jsonInput.insert("select", QString{select ? "true" : "false"});
	jsonInput.insert("print", QString{print ? "true" : "false"});

	return read_sync<void>(QUrl(url), { 200, 202 },
			[&](QNetworkRequest& request) {
				request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
				request.setRawHeader(QByteArray{"X-Api-Key"}, QByteArray{api_key().c_str()});
			},
			[&](QNetworkAccessManager& nam, QNetworkRequest& request) {
				return nam.post(request, QJsonDocument(jsonInput).toJson());
			},
			[](QNetworkReply *reply) {
				const auto doc = QJsonDocument::fromJson(reply->readAll());
				PRINTDB("Response: %s", QString{doc.toJson()}.toStdString());
				PRINT("Slice command successfully executed");
			}
	);
}
