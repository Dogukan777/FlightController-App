#include "websocketclient.h"

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject{parent}
{
    connect(&m_socket, &QWebSocket::connected,
            this, &WebSocketClient::onConnected);

    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessage);

    connect(&m_socket, &QWebSocket::binaryMessageReceived,
            this, &WebSocketClient::onBinaryMessage);

    connect(&m_socket, &QWebSocket::errorOccurred,
            this, &WebSocketClient::onError);
}
void WebSocketClient::connectToServer()
{
    QUrl url(QStringLiteral("ws://172.20.10.3:3030"));
    qDebug() << "Connecting to" << url;
    m_socket.open(url);
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected!";
    // Gerekirse burada ilk mesajını gönderebilirsin
    // m_socket.sendTextMessage("Hello server");
}
void WebSocketClient::onTextMessage(const QString &message)
{
    qDebug() << "Text message received:" << message;
    emit textMessageReceived(message);
}
void WebSocketClient::onBinaryMessage(const QByteArray &message)
{
    qDebug() << "Binary message received, size =" << message.size();

    // 1) JPEG olarak yüklemeyi dene
    QImage img;
    bool ok = img.loadFromData(message, "JPG");   // ya da "JPEG"

    if (!ok || img.isNull()) {
        qWarning() << "Failed to decode image from WebSocket data";
        // istersen burada bitsReceived ile debug amaçlı bit string basmaya devam edebilirsin
        return;
    }

    // 2) UI'ya göndermek için sinyal yay
    emit imageReceived(img);
}



void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "WebSocket error:" << m_socket.errorString();
    emit errorOccurred(m_socket.errorString());
}











