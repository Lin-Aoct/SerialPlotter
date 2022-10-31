#include "serialsettingwindow.hpp"
#include "ui_serialsettingwindow.h"

SerialSetting::SerialSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SerialSetting)
{
    ui->setupUi(this);
    setWindowTitle("更多串口设置");

    /* Populate data bits combo box */
    ui->comboData->addItem("8 bits");
    ui->comboData->addItem("7 bits");

    /* Populate parity combo box */
    ui->comboParity->addItem("none");
    ui->comboParity->addItem("odd");
    ui->comboParity->addItem("even");

    /* Populate stop bits combo box */
    ui->comboStop->addItem("1 bit");
    ui->comboStop->addItem("2 bits");
}

SerialSetting::~SerialSetting()
{
    delete ui;
}

/**
 * @brief 调用此函数设置默认显示参数
 */
void SerialSetting::set_param(QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity,
                        QSerialPort::StopBits stopBits)
{
    switch(dataBits)
    {
        case QSerialPort::Data8:
        {
            ui->comboData->setCurrentIndex(0);
            break;
        }
        case QSerialPort::Data7:
        {
            ui->comboData->setCurrentIndex(1);
            break;
        }
        default:
            break;
    }

    switch(parity)
    {
        case QSerialPort::NoParity:
        {
            ui->comboParity->setCurrentIndex(0);
            break;
        }
        case QSerialPort::OddParity:
        {
            ui->comboParity->setCurrentIndex(1);
            break;
        }
        case QSerialPort::EvenParity:
        {
            ui->comboParity->setCurrentIndex(2);
            break;
        }
        default:
            break;
    }

    switch(stopBits)
    {
        case QSerialPort::OneStop:
        {
            ui->comboStop->setCurrentIndex(0);
            break;
        }
        case QSerialPort::TwoStop:
        {
            ui->comboStop->setCurrentIndex(1);
            break;
        }
        default:
            break;
    }

}

/**
 * @brief <槽函数> 确认或取消按钮 点击事件
 */
void SerialSetting::on_buttonBox_clicked(QAbstractButton *button)
{
    if(this->ui->buttonBox->standardButton(button) != QDialogButtonBox::Ok)
        return;

    int dataBitsIndex = ui->comboData->currentIndex();       // Get index of data bits combo box
    int parityIndex = ui->comboParity->currentIndex();       // Get index of parity combo box
    int stopBitsIndex = ui->comboStop->currentIndex();       // Get index of stop bits combo box
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;

    /* Set data bits according to the selected index */
    switch(dataBitsIndex)
    {
        case 0:
            dataBits = QSerialPort::Data8;
            break;
        default:
            dataBits = QSerialPort::Data7;
    }

    /* Set parity according to the selected index */
    switch(parityIndex)
    {
        case 0:
            parity = QSerialPort::NoParity;
            break;
        case 1:
            parity = QSerialPort::OddParity;
            break;
        default:
            parity = QSerialPort::EvenParity;
    }

    /* Set stop bits according to the selected index */
    switch(stopBitsIndex)
    {
        case 0:
            stopBits = QSerialPort::OneStop;
            break;
        default:
            stopBits = QSerialPort::TwoStop;
    }

    emit okPressed(dataBits, parity, stopBits);
}

