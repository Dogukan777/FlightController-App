#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include "home.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Main;
}
QT_END_NAMESPACE

class Main : public QMainWindow
{
    Q_OBJECT

public:
    Main(QWidget *parent = nullptr);
    ~Main();

private:

    Ui::Main *ui;
    Home *homeWin = nullptr;
    void addStyleSheet();

private slots:
    void on_btnUAV_clicked();
    void on_btnUGV_clicked();

};
#endif // MAIN_H
