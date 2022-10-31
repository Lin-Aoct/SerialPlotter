#ifndef SERIALSETTINGWINDOW_HPP
#define SERIALSETTINGWINDOW_HPP

#include <QDialog>
#include <qserialport.h>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QtDebug>
#include "com.h"

namespace Ui
{
    class SerialSetting;
}

class SerialSetting : public QDialog
{
    Q_OBJECT

public:
    explicit SerialSetting(QWidget *parent = nullptr);
    ~SerialSetting();
    void set_param(QSerialPort::DataBits dataBits,
                   QSerialPort::Parity parity,
                   QSerialPort::StopBits stopBits);

private:
    Ui::SerialSetting *ui;

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

signals:
    void okPressed(QSerialPort::DataBits,
                           QSerialPort::Parity,
                           QSerialPort::StopBits);
};

#endif // SerialSETTINGWINDOW_HPP
