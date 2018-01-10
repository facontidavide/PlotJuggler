/****************************************************************************
**
** Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebSockets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Client.h"
#include <QtCore/QDebug>
#include <QtMath>
#include <QThread>
#include <chrono>
#include <QStringBuilder>
#include <Windows.h>
#include <thread>

QT_USE_NAMESPACE

//! [constructor]
Client::Client(const QUrl &url, bool debug, QObject *parent) :
    QObject(parent),
    m_url(url),
    m_debug(debug),timer_(this),timer2_(this)
{
    if (m_debug)
        qDebug() << "WebSocket server:" << url;
    connect(&m_webSocket, &QWebSocket::connected, this, &Client::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &Client::closed);
    m_webSocket.open(QUrl(url));
}
//! [constructor]

//! [onConnected]
void Client::onConnected()
{
    if (m_debug)
        qDebug() << "WebSocket connected";
    m_webSocket.sendTextMessage("start");

    i_ = 0.0;

    timer_.setInterval(50);
    timer2_.setInterval(10);
    double i=0.0;
    connect(&timer_, &QTimer::timeout, [&](){
        sendMsg("sin");
    });
    connect(&timer2_, &QTimer::timeout, [&](){
        sendMsg("cos");
    });

    timer_.start();
    timer2_.start();
}

void Client::sendMsg(const QString &key){
    using namespace std::chrono;
    static std::chrono::high_resolution_clock::time_point initial_time = high_resolution_clock::now();

    double steps = 1000.0;
    i_ +=M_PI/steps;    

     double value = qSin(i_);
     if(key=="cos")
         value = qCos(i_);
     auto now =  high_resolution_clock::now();
     const double t = duration_cast< duration<double>>( now - initial_time ).count() ;
     QString str = key+QString(":")
             + QString::number(t)+":"
             + QString::number(value);
     qDebug() << str;
     m_webSocket.sendTextMessage(str);
}
//! [onConnected]

//! [onTextMessageReceived]
void Client::onTextMessageReceived(QString message)
{
    if (m_debug)
        qDebug() << "Message received:" << message;
    m_webSocket.close();
}
//! [onTextMessageReceived]
