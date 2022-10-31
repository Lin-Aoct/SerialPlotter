/***************************************************************************
**  This file is part of Serial Port Plotter                              **
**                                                                        **
**                                                                        **
**  Serial Port Plotter is a program for bPlotting integer data from       **
**  serial port using Qt and QCustomPlot                                  **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Borislav                                             **
**           Contact: b.kereziev@gmail.com                                **
**           Date: 29.12.14                                               **
****************************************************************************/

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "RTT/rttjlink.hpp"
#include "com.h"
#include "helpwindow.hpp"
#include "qcustomplot/qcustomplot.h"
#include "serialsettingwindow.hpp"
#include <QMainWindow>
#include <QSerialPortInfo>
#include <QtSerialPort/QtSerialPort>

#define VERSION "0.2.2"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define START_MSG '$'
#define END_MSG ';'

#define WAIT_START 1
#define IN_MESSAGE 2
#define UNDEFINED 3

#define CUSTOM_LINE_COLORS 14
#define GCP_CUSTOM_LINE_COLORS 4

typedef struct _Serial_Info
{
    QSerialPort *serialPort;
    QSerialPort::DataBits curSerialDataBits = COM_DEFAULT_DATA_BITS;
    QSerialPort::Parity curSerialParity = COM_DEFAULT_PARITY;
    QSerialPort::StopBits curSerialStopBits = COM_DEFAULT_STOP_BITS;
    Serial_Decode_Type comDecodeType;
    int serialRxCnt = 0;
    int serialTxCnt = 0;
} Serial_Info;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_comboPort_currentIndexChanged(const QString &arg1);  // Slot displays message on status bar
    void portOpenedSuccess();                                    // Called when port opens OK
    void portOpenedFail();                                       // Called when port fails to open
    void onPortClosed();                                         // Called when closing the port
    void replot();                                               // Slot for repainting the plot
    void onNewDataArrived(QStringList newData);                  // Slot for new data from serial port
    void saveStream(QStringList newData);                        // Save the received data to the opened file
    void on_spinAxesMin_valueChanged(int arg1);                  // Changing lower limit for the plot
    void on_spinAxesMax_valueChanged(int arg1);                  // Changing upper limit for the plot
    void readData();                                             // Slot for inside serial port
    void on_comboBaud_currentTextChanged(const QString &text);
    void serial_setting_window_handle(QSerialPort::DataBits, QSerialPort::Parity, QSerialPort::StopBits);

    // void on_comboAxes_currentIndexChanged(int index);                                     // Display number of axes and colors in status bar
    void on_spinYStep_valueChanged(int arg1);                   // Spin box for changing Y axis tick step
    void on_savePNGButton_clicked();                            // Button for saving JPG
    void onMouseMoveInPlot(QMouseEvent *event);                 // Displays coordinates of mouse pointer when clicked in plot in status bar
    void onMousePressInPlot(QMouseEvent *event);
    void onMouseReleaseInPlot(QMouseEvent *event);
    void onAfterPlot();
    void on_spinPoints_valueChanged(int arg1);                  // Spin box controls how many data points are collected and displayed
    void onMouseWhellInPlot(QWheelEvent *event);                // Makes wheel mouse works while bPlotting

    /* Used when a channel is selected (plot or legend) */
    void channel_selection(void);
    void legend_click(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);
    void legend_double_click(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);

    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionHow_to_use_triggered();
    void on_actionPausePlot_triggered();
    void on_actionClear_triggered();
    void on_actionRecord_stream_triggered();

    void on_pushButton_TextEditHide_clicked();

    void on_pushButton_ShowallData_clicked();

    void on_pushButton_AutoScale_clicked();

    void on_pushButton_ResetVisible_clicked();

    void on_listWidget_Channels_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_Refresh_clicked();
    void on_pushButton_MoreUartSetting_clicked();

    void on_pushButton_ClearTextEdit_clicked();

    void on_pushButton_AutoScroll_clicked();

    void on_checkBox_ShowDataPoint_stateChanged(int state);
    void on_checkBox_UartDecodeWithHex_stateChanged(int state);

signals:
    void portOpenFail();             // Emitted when cannot open port
    void portOpenOK();               // Emitted when port is open
    void portClosed();               // Emitted when port is closed
    void newData(QStringList data);  // Emitted when new data has arrived

private:
    Ui::MainWindow *ui;

    /* Line colors */
    QColor line_colors[CUSTOM_LINE_COLORS];
    QColor gui_colors[GCP_CUSTOM_LINE_COLORS];

    /* Main info */
    bool bSerialConnected;  // Status connection variable
    bool bPlotting;         // Status bPlotting variable
    int dataPointNumber;    // Keep track of data points
    /* Channels of data (number of graphs) */
    int channels;

    /* Data format */
    int data_format;

    /* Textbox Related */
    bool filterDisplayedData = true;

    bool bPlotZoomWithWheel = false;

    bool bMousePressedInPlot = false;           // 标志鼠标是否已在图表中按下
    bool bShiftModifierPressedInPlot = false;   // 标志 Shift 键是否已在图表中按下

    /* Listview Related */
    QStringListModel *channelListModel;
    QStringList channelStrList;

    //-- CSV file to save data
    QFile *m_csvFile = nullptr;
    void openCsvFile(void);
    void closeCsvFile(void);

    QCPItemTracer *plotTracer;
    SerialSetting *serialSettingWindow;
    QLabel *uartCntLabel;
    Serial_Info serialInfo;

    QTimer updateTimer;         // Timer used for replotting the plot
    QTime timeOfFirstData;      // Record the time of the first data point
    double timeBetweenSamples;  // Store time between samples
    QString receivedData;       // Used for reading from the port
    int STATE;                  // State of recieiving message from port
    int NUMBER_OF_POINTS;       // Number of points plotted
    HelpWindow *helpWindow;

    void createUI();                        // Populate the controls
    void enable_com_controls(bool enable);  // Enable/disable controls
    void setupPlot();                       // Setup the QCustomPlot
                                            // Open the inside serial port with these parameters
    void openPort(QSerialPortInfo portInfo, int baudRate, QSerialPort::DataBits dataBits, QSerialPort::Parity parity, QSerialPort::StopBits stopBits);

    RTTJLink *RTTJLinkHandle;
    enum Except
    {
        EXCEP_ZERO,
        EXCEP_ONE
    };
};

#endif  // MAINWINDOW_HPP
