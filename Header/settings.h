#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include "SerialManager.h"
#include <QElapsedTimer>

namespace Ui {
class Settings;
}
struct servoSettings {
    int servoId;
    int servoMaxValue;
    int servoMinValue;
    int servoValue;
};
class Settings : public QWidget
{
    Q_OBJECT

public:
    explicit Settings(SerialManager* serialPtr,const QVector<servoSettings>& servoList, QWidget *parent = nullptr);
    ~Settings();
    void addStyleSheet();
    void listSerialPorts();
    void getTriggers();
    void onSerialMessage(const QString &port, const QString &msg);

private:
    Ui::Settings *ui;
    SerialManager *serial;
    QVector<servoSettings> servos;
    bool isConnected = false;
    bool wpReading = false;
    bool wpRequestedOnce = false;
    bool dataRequestedOnce = false;
    QString currentPort;
    QString rxBuffer;
    QElapsedTimer yawTimer;
    bool yawTimerStarted = false;


private slots:
    void onSendClicked();
    void onReadClicked();


};



#endif // SETTINGS_H
