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

#pragma once

#include <QString>
#include <QtNetwork>

#include "printutils.h"
#include "PlatformUtils.h"

class NetworkException: public std::exception
{
public:
	NetworkException(const QNetworkReply::NetworkError& error, const QString& errorMessage) : error(error), errorMessage(errorMessage) { }
	virtual ~NetworkException() {}

	const QNetworkReply::NetworkError& getError() const { return error; }
	const QString& getErrorMessage() const { return errorMessage; }

	virtual const char* what() const throw()
	{
		return errorMessage.toStdString().c_str();
	}

private:
	QNetworkReply::NetworkError error;
	QString errorMessage;
};

template <typename ResultType>
class NetworkRequest
{
public:
	using setup_func_t = std::function<void(QNetworkRequest&)>;
	using reply_func_t = std::function<QNetworkReply*(QNetworkAccessManager&, QNetworkRequest&)>;
	using transform_func_t = std::function<ResultType(QNetworkReply *)>;

	NetworkRequest(const QUrl url, const std::vector<int> accepted_codes, const int timeout_seconds) : url(url), accepted_codes(accepted_codes), timeout_seconds(timeout_seconds) { }
	virtual ~NetworkRequest() { }

	ResultType execute(setup_func_t setup_func, reply_func_t reply_func, transform_func_t transform_func);

private:
	QUrl url;
	std::vector<int> accepted_codes;
	int timeout_seconds;
};

template <typename ResultType>
ResultType NetworkRequest<ResultType>::execute(NetworkRequest::setup_func_t setup_func, NetworkRequest::reply_func_t reply_func, NetworkRequest::transform_func_t transform_func)
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
	timer.start(timeout_seconds * 1000);
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
