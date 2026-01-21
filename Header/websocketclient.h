#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QImage>
class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    void connectToServer();

signals:
    void textMessageReceived(const QString &text);
    void bitsReceived(const QString &bits);      // İstersen bunu artık kullanmayabilirsin
    void imageReceived(const QImage &image);     // *** YENİ ***
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onTextMessage(const QString &message);
    void onBinaryMessage(const QByteArray &message);
    void onError(QAbstractSocket::SocketError error);
private:
    QWebSocket m_socket;
};

#endif // WEBSOCKETCLIENT_H
