#pragma once

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QAbstractSocket>

class WebsocketClient : public QObject
{
    Q_OBJECT

public:
    WebsocketClient();
    ~WebsocketClient();

    bool start();
    void shutdown();

signals:
    void closed();

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

private:
    QWebSocket _socket;
    QUrl _url;
    bool _running;
};
