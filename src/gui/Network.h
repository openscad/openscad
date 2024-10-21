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

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <algorithm>
#include <functional>
#include <exception>
#include <QObject>
#include <QString>
#include <QtNetwork>

#include <utility>
#include <string>
#include <vector>

#include "utils/printutils.h"
#include "platform/PlatformUtils.h"
#include "gui/NetworkSignal.h"

class NetworkException : public std::exception
{
public:
  NetworkException(const QNetworkReply::NetworkError& error, const QString& errorMessage) : error(error), errorMessage(errorMessage.toStdString()) { }

  const QNetworkReply::NetworkError& getError() const { return error; }
  const std::string& getErrorMessage() const { return errorMessage; }

  const char *what() const noexcept override
  {
    return errorMessage.c_str();
  }

private:
  QNetworkReply::NetworkError error;
  std::string errorMessage;
};

using network_progress_func_t = std::function<bool (double)>;

template <typename ResultType>
class NetworkRequest
{
public:
  using setup_func_t = std::function<void (QNetworkRequest&)>;
  using reply_func_t = std::function<QNetworkReply *(QNetworkAccessManager&, QNetworkRequest&)>;
  using transform_func_t = std::function<ResultType (QNetworkReply *)>;

  NetworkRequest(QUrl url, std::vector<int> accepted_codes, const int timeout_seconds)
    : url(std::move(url)), accepted_codes(std::move(accepted_codes)), timeout_seconds(timeout_seconds)
  { }
  virtual ~NetworkRequest() = default;

  void set_progress_func(const network_progress_func_t& progress_func) { this->progress_func = progress_func; }
  ResultType execute(const setup_func_t& setup_func, const reply_func_t& reply_func, transform_func_t transform_func);

private:
  QUrl url;
  std::vector<int> accepted_codes;
  int timeout_seconds;
  network_progress_func_t progress_func = nullptr;
};

template <typename ResultType>
ResultType NetworkRequest<ResultType>::execute(const NetworkRequest::setup_func_t& setup_func,
    const NetworkRequest::reply_func_t& reply_func,
    NetworkRequest::transform_func_t transform_func)
{
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromStdString(PlatformUtils::user_agent()));
  setup_func(request);

  QNetworkAccessManager nam;
  QNetworkReply *reply = reply_func(nam, request);

  QTimer timer;
  QEventLoop loop;
  NetworkSignal forwarder{nullptr, [&](qint64 bytesSent, qint64 bytesTotal) {
                            const double permille = (1000.0 * bytesSent) / bytesTotal;
                            timer.start();
                            if (progress_func && progress_func(permille)) {
                              reply->abort();
                            }
                          }};
  QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
  QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  QObject::connect(reply, SIGNAL(uploadProgress(qint64,qint64)), &forwarder, SLOT(network_progress(qint64,qint64)));
  timer.setSingleShot(true);
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
