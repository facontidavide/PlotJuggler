#include <QApplication>
#include "websocket_client.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WebsocketClient client;
    client.start();

    return app.exec();
}
