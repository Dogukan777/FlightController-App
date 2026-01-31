#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QByteArray>

class SerialManager : public QObject
{
    Q_OBJECT
public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();
    bool connectSerial(const QString &portName);
    bool send(const QString &portName, const QString &message);
    QString receive(const QString &portName);
    void disconnectSerial(QString portName);
    bool isConnected() const;
    QString currentPortName() const;
    void setPortName(QString portName);
    void clearRx();

signals:
    void connected(const QString &portName);
    void disconnected(const QString &portName);
    void errorOccurred(const QString &portName, const QString &errorString);
    void messageReceived(const QString &portName, const QString &message);

private slots:
    void onReadyRead();

private:
    bool ensureConnectedTo(const QString &portName);
    qint32 boundRate = 115200;

private:
    QSerialPort *m_serial;
    QByteArray   m_rxBuffer;
    QString portName;

};


#endif // SERIALMANAGER_H
