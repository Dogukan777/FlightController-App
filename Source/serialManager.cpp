#include "SerialManager.h"
#include <QDebug>

SerialManager::SerialManager(QObject *parent)
    : QObject(parent),
    m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialManager::onReadyRead);
}

SerialManager::~SerialManager()
{
    disconnectSerial(this->portName);
}

bool SerialManager::connectSerial(const QString &portName)
{
    const QString p = portName.trimmed();
    if (p.isEmpty()) {
        emit errorOccurred(p, "Port name is empty.");
        return false;
    }else{
        setPortName(portName);
    }

    // Zaten aynı porta bağlıysa
    if (m_serial->isOpen() && m_serial->portName() == p) {
        return true;
    }

    // Başka porta açıksa kapat
    if (m_serial->isOpen()) {
        m_serial->close();
        emit disconnected(m_serial->portName());
    }

    m_serial->setPortName(p);

    m_serial->setBaudRate(this->boundRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(p, m_serial->errorString());
        return false;
    }

    m_rxBuffer.clear();
    emit connected(p);
    return true;
}
void SerialManager::clearRx()
{


    if (m_serial && m_serial->isOpen()){
        m_rxBuffer.clear();
        m_serial->readAll();

    }
}

void SerialManager::setPortName(QString portName){
    this->portName=portName;
}

bool SerialManager::ensureConnectedTo(const QString &portName)
{
    const QString p = portName.trimmed();

    if (p.isEmpty()) {
        emit errorOccurred(p, "Port name is empty.");
        return false;
    }

    if (m_serial->isOpen() && m_serial->portName() == p) {
        return true;
    }

    // Bağlı değilse bağlanmayı dene (default 115200)
    return connectSerial(p);
}

bool SerialManager::send(const QString &portName, const QString &message)
{
    if (!ensureConnectedTo(portName)) {
        return false;
    }

    QByteArray data = message.toUtf8();

    // Seri hab. için çoğu zaman satır sonu iyi olur (isteğe bağlı)
    if (!data.endsWith('\n')) data.append('\n');

    const qint64 written = m_serial->write(data);
    if (written < 0) {
        emit errorOccurred(m_serial->portName(), m_serial->errorString());
        return false;
    }

    // Tam gönderim için kısa bekleme (opsiyonel ama pratik)
    if (!m_serial->waitForBytesWritten(100)) {
        // Her zaman hata değildir ama bilgilendirelim
        qWarning() << "waitForBytesWritten timeout:" << m_serial->portName();
    }

    return true;
}

QString SerialManager::receive(const QString &portName)
{
    if (!ensureConnectedTo(portName)) {
        return {};
    }

    // Buffer'da biriken veriyi döndürür ve buffer'ı temizler
    const QString msg = QString::fromUtf8(m_rxBuffer);
    m_rxBuffer.clear();
    return msg;
}

void SerialManager::onReadyRead()
{
    m_rxBuffer.append(m_serial->readAll());

    int nl;
    while ((nl = m_rxBuffer.indexOf('\n')) != -1)
    {
        QByteArray line = m_rxBuffer.left(nl);
        m_rxBuffer.remove(0, nl + 1);

        line = line.trimmed();
        if (line.isEmpty()) continue;

        // HER EMIT = TEK SATIR
        emit messageReceived(m_serial->portName(),
                             QString::fromUtf8(line));
    }
}



void SerialManager::disconnectSerial(QString portName)
{
    if (m_serial->isOpen()) {
        m_serial->close();
        emit disconnected(portName);
    }
}

bool SerialManager::isConnected() const
{
    return m_serial->isOpen();
}

QString SerialManager::currentPortName() const
{
    return m_serial->portName();
}
