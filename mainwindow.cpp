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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <x86intrin.h>


/**
 * @brief Constructor
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      /* Populate colors */
      line_colors{
          /* For channel data (gruvbox palette) */
          /* Light */
          QColor("#fb4934"),
          QColor("#b8bb26"),
          QColor("#fabd2f"),
          QColor("#83a598"),
          QColor("#d3869b"),
          QColor("#8ec07c"),
          QColor("#fe8019"),
          /* Light */
          QColor("#cc241d"),
          QColor("#98971a"),
          QColor("#d79921"),
          QColor("#458588"),
          QColor("#b16286"),
          QColor("#689d6a"),
          QColor("#d65d0e"),
      },
      gui_colors{
          /* Monochromatic for axes and ui */
          QColor(48, 47, 47, 255),    /**<  0: qdark ui dark/background color */
          QColor(80, 80, 80, 255),    /**<  1: qdark ui medium/grid color */
          QColor(170, 170, 170, 255), /**<  2: qdark ui light/text color */
          QColor(48, 47, 47, 200)     /**<  3: qdark ui dark/background color w/transparency */
      },
      /* Main vars */
      bSerialConnected(false), bPlotting(false), dataPointNumber(0), channels(0), STATE(WAIT_START), NUMBER_OF_POINTS(500)
{
    ui->setupUi(this);

    /* Init UI and populate UI controls */
    createUI();

    /* Setup plot area and connect controls slots */
    setupPlot();

    // 连接绘图区域鼠标滚轮滚动事件信号
    connect(ui->plot, SIGNAL(mouseWheel(QWheelEvent *)), this, SLOT(onMouseWhellInPlot(QWheelEvent *)));

    // 连接绘图区域鼠标按下事件信号
    connect(ui->plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(onMousePressInPlot(QMouseEvent *)));

    // 连接绘图区域鼠标释放事件信号
    connect(ui->plot, SIGNAL(mouseRelease(QMouseEvent *)), this, SLOT(onMouseReleaseInPlot(QMouseEvent *)));

    // 连接绘图区域鼠标移动事件信号
    connect(ui->plot, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(onMouseMoveInPlot(QMouseEvent *)));

    // connect(ui->plot, SIGNAL(afterReplot()), this, SLOT(onAfterPlot()));

    /* Channel selection */
    connect(ui->plot, SIGNAL(selectionChangedByUser()), this, SLOT(channel_selection()));
    connect(ui->plot,
            SIGNAL(legendClick(QCPLegend *, QCPAbstractLegendItem *, QMouseEvent *)),
            this,
            SLOT(legend_click(QCPLegend *, QCPAbstractLegendItem *, QMouseEvent *)));
    connect(ui->plot,
            SIGNAL(legendDoubleClick(QCPLegend *, QCPAbstractLegendItem *, QMouseEvent *)),
            this,
            SLOT(legend_double_click(QCPLegend *, QCPAbstractLegendItem *, QMouseEvent *)));

    // 连接重绘更新定时器函数
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(replot()));

    connect(ui->checkBox_ShowDataPoint,
            &QCheckBox::stateChanged,
            this,
            &MainWindow::on_checkBox_ShowDataPoint_stateChanged);

    connect(ui->checkBox_UartDecodeWithHex,
            &QCheckBox::stateChanged,
            this,
            &MainWindow::on_checkBox_UartDecodeWithHex_stateChanged);

    m_csvFile = nullptr;

}

/**
 * @brief 主窗口析构函数
 */
MainWindow::~MainWindow()
{
    closeCsvFile();

    if(this->serialInfo.serialPort != nullptr)
    {
        delete this->serialInfo.serialPort;
    }
    delete ui;
}

/**
 * @brief Create remaining elements and populate the controls
 */
void MainWindow::createUI()
{

    /* Populate baud rate combo box with standard rates */
    ui->comboBaud->addItem("1200");
    ui->comboBaud->addItem("2400");
    ui->comboBaud->addItem("4800");
    ui->comboBaud->addItem("9600");
    ui->comboBaud->addItem("19200");
    ui->comboBaud->addItem("38400");
    ui->comboBaud->addItem("57600");
    ui->comboBaud->addItem("115200");
    /* And some not-so-standard */
    ui->comboBaud->addItem("128000");
    ui->comboBaud->addItem("153600");
    ui->comboBaud->addItem("230400");
    ui->comboBaud->addItem("256000");
    ui->comboBaud->addItem("460800");
    ui->comboBaud->addItem("921600");

    /* Select 115200 bits by default */
    ui->comboBaud->setCurrentIndex(7);

    /* Initialize the listwidget */
    ui->listWidget_Channels->clear();

    // 检查是否存在串口设备
    if(QSerialPortInfo::availablePorts().size() == 0)
    {
        enable_com_controls(false);
        ui->actionPausePlot->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
        ui->savePNGButton->setEnabled(false);
        ui->statusBar->showMessage("未找到串口设备");
    }
    else
    {
        // 遍历串口设备并将设备信息赋值到控件
        for(QSerialPortInfo port : QSerialPortInfo::availablePorts())
        {
            ui->comboPort->addItem(port.portName() + " " + port.description());
        }
    }

    // 右下角显示收发字节统计
    this->uartCntLabel = new QLabel(QString("R: 0  |  T: 0"));
    ui->statusBar->insertPermanentWidget(0, uartCntLabel);
}

/**
 * @brief Setup the plot area
 */
void MainWindow::setupPlot()
{
    /* 清空图表 */
    ui->plot->clearItems();

    /* 设置背景颜色 */
    ui->plot->setBackground(gui_colors[0]);

    /* Used for higher performance (see QCustomPlot real time example) */
    ui->plot->setNotAntialiasedElements(QCP::aeAll);
    QFont font;
    font.setStyleStrategy(QFont::NoAntialias);
    ui->plot->legend->setFont(font);

    /* X 轴样式 */
    ui->plot->xAxis->grid()->setPen(QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridPen(QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->xAxis->grid()->setSubGridVisible(true);
    ui->plot->xAxis->setBasePen(QPen(gui_colors[2]));
    ui->plot->xAxis->setTickPen(QPen(gui_colors[2]));
    ui->plot->xAxis->setSubTickPen(QPen(gui_colors[2]));
    ui->plot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->xAxis->setTickLabelColor(gui_colors[2]);
    ui->plot->xAxis->setTickLabelFont(font);
    /* X 范围 */
    ui->plot->xAxis->setRange(dataPointNumber - ui->spinPoints->value(), dataPointNumber);

    /* Y 轴样式 */
    ui->plot->yAxis->grid()->setPen(QPen(gui_colors[2], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridPen(QPen(gui_colors[1], 1, Qt::DotLine));
    ui->plot->yAxis->grid()->setSubGridVisible(true);
    ui->plot->yAxis->setBasePen(QPen(gui_colors[2]));
    ui->plot->yAxis->setTickPen(QPen(gui_colors[2]));
    ui->plot->yAxis->setSubTickPen(QPen(gui_colors[2]));
    ui->plot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->plot->yAxis->setTickLabelColor(gui_colors[2]);
    ui->plot->yAxis->setTickLabelFont(font);
    /* Y 范围 */
    ui->plot->yAxis->setRange (ui->spinAxesMin->value(), ui->spinAxesMax->value());
    /* User can change Y axis tick step with a spin box */
    // ui->plot->yAxis->setAutoTickStep (false);
    // ui->plot->yAxis->(ui->spinYStep->value());

    /* User interactions Drag and Zoom are allowed only on X axis, Y is fixed
     * manually by UI control */
    ui->plot->setInteraction(QCP::iRangeDrag, true);                    // 允许拖动
    ui->plot->setInteraction (QCP::iRangeZoom, false);                  // 不允许原生放大
    ui->plot->setInteraction(QCP::iSelectPlottables, true);             // Plottable 可选中
    ui->plot->setInteraction(QCP::iSelectLegend, true);                 // 图例可选中
    ui->plot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);  // 轴拖动方向
    ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);                 // 轴缩放方向
    ui->plot->axisRect()->setRangeZoomFactor(0.975, 0.0);               // 轴缩放因子
//    ui->plot->axisRect()->setBackgroundScaledMode(true);
//    Qt::AspectRatioMode::KeepAspectRatioByExpanding;

    // 图例设置
    QFont legendFont;
    legendFont.setPointSize(9);
    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->legend->setBrush(gui_colors[3]);
    ui->plot->legend->setBorderPen(gui_colors[2]);
    /* By default, the legend is in the inset layout of the main axis rect. So
     * this is how we access it to change legend placement */
    ui->plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    // 十字浮标追踪绘图窗口
    plotTracer = new QCPItemTracer(ui->plot);
    // 设置十字浮标样式
    QPen *pen = new QPen;
    pen->setColor(QColor(Qt::white));   // 白色
    pen->setStyle(Qt::DashLine);        // 点线
    plotTracer->setPen(*pen);
}

/**
 * @brief Enable/disable COM controls
 * @param enable true enable, false disable
 */
void MainWindow::enable_com_controls(bool enable)
{
    // 设置工具栏项目属性
    ui->actionConnect->setEnabled(enable);
    ui->actionPausePlot->setEnabled(!enable);
    ui->actionDisconnect->setEnabled(!enable);
}

/**
 * @brief Open the inside serial port; connect its signals
 * @param portInfo
 * @param baudRate
 * @param dataBits
 * @param parity
 * @param stopBits
 */
void MainWindow::openPort(QSerialPortInfo portInfo,
                          int baudRate,
                          QSerialPort::DataBits dataBits,
                          QSerialPort::Parity parity,
                          QSerialPort::StopBits stopBits)
{
    this->serialInfo.serialPort = new QSerialPort(portInfo, nullptr);  // Create a new serial port

    connect(this, SIGNAL(portOpenOK()), this,
            SLOT(portOpenedSuccess()));  // Connect port signals to GUI slots
    connect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    connect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    connect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));
    connect(this->serialInfo.serialPort, SIGNAL(readyRead()), this, SLOT(readData()));

    connect(this, SIGNAL(newData(QStringList)), this, SLOT(saveStream(QStringList)));

    connect(this->ui->comboBaud,
            &QComboBox::currentTextChanged,
            this,
            &MainWindow::on_comboBaud_currentTextChanged);

    if(this->serialInfo.serialPort->open(QIODevice::ReadWrite))
    {
        this->serialInfo.serialPort->setBaudRate(baudRate);
        this->serialInfo.serialPort->setParity(parity);
        this->serialInfo.serialPort->setDataBits(dataBits);
        this->serialInfo.serialPort->setStopBits(stopBits);
        emit portOpenOK();
    }
    else
    {
        emit portOpenedFail();
        qDebug() << this->serialInfo.serialPort->errorString();
    }
}

/**
 * @brief <槽函数> 端口关闭 事件回调函数
 */
void MainWindow::onPortClosed()
{
    updateTimer.stop();
    bSerialConnected = false;
    bPlotting = false;

    closeCsvFile();

    disconnect(this->serialInfo.serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
    disconnect(this, SIGNAL(portOpenOK()), this,
               SLOT(portOpenedSuccess()));  // Disconnect port signals to GUI slots
    disconnect(this, SIGNAL(portOpenFail()), this, SLOT(portOpenedFail()));
    disconnect(this, SIGNAL(portClosed()), this, SLOT(onPortClosed()));
    disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(onNewDataArrived(QStringList)));

    disconnect(this, SIGNAL(newData(QStringList)), this, SLOT(saveStream(QStringList)));
}

/**
 * @brief Port Combo Box index changed slot; displays info for selected port
 * when combo box is changed
 * @param arg1
 */
void MainWindow::on_comboPort_currentIndexChanged(const QString &arg1)
{
    QSerialPortInfo selectedPort(arg1);  // Dislplay info for selected port
    ui->statusBar->showMessage(selectedPort.description());
}

/**
 * @brief <槽函数> 端口打开成功 事件回调函数
 */
void MainWindow::portOpenedSuccess()
{
    setupPlot();  // Create the QCustomPlot area
    ui->statusBar->showMessage("已连接");
    enable_com_controls(false);  // Disable controls if port is open

    // 判断是否需要记录数据
    if(ui->actionRecord_stream->isChecked())
    {
        //--> Create new CSV file with current date/timestamp
        openCsvFile();
    }
    // 正在接收数据时禁用记录数据按钮
    ui->actionRecord_stream->setEnabled(false);

    updateTimer.start(20);  // Slot is refreshed 20 times per second
    bSerialConnected = true;       // Set flags
    bPlotting = true;
}

/**
 * @brief <槽函数> 端口打开失败 事件回调函数
 */
void MainWindow::portOpenedFail()
{
    ui->statusBar->showMessage("端口打开失败，请检查是否被占用!");
}

/**
 * @brief <槽函数> 波特率选择框文本改变 事件回调函数
 */
void MainWindow::on_comboBaud_currentTextChanged(const QString &text)
{
    // 判断当前是否已打开串口
    if(this->bSerialConnected)
    {
        this->serialInfo.serialPort->setBaudRate(text.toInt());
    }
}

/**
 * @brief <定时器槽函数> 图表重绘
 */
void MainWindow::replot()
{
    // 判断是否需要 X 轴自动滚动
    if(ui->checkBox_XAxisAutoScroll->isChecked())
    {
        ui->plot->xAxis->setRange(dataPointNumber - ui->spinPoints->value(), dataPointNumber);
    }
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

/**
 * @brief Slot for new data from serial port . Data is comming in QStringList
 * and needs to be parsed
 * @param newData
 */
void MainWindow::onNewDataArrived(QStringList newData)
{
    static int dataMembers = 0;
    int channel = 0;
    static int i = 0;
//    volatile bool youShallNotPass = false;

    /* When a fast baud rate is set (921kbps was the first to starts to
       bug), this method is called multiple times (2x in the 921k tests), so
       a flag is used to throttle TO-DO: Separate processes, buffer data (1)
       and process data (2) */
//    while(youShallNotPass)
//    {
//    }
//    youShallNotPass = true;

    if(bPlotting)
    {
        /* Get size of received list */
        dataMembers = newData.size();

        // 解析数据
        for(i = 0; i < dataMembers; i++)
        {
            // 判断是否有新的通道数据 是则创建新的图表绘图通道
            while(ui->plot->plottableCount() <= channel)
            {
                // 创建新的通道
                ui->plot->addGraph();
                ui->plot->graph()->setPen(QPen(line_colors[channels % CUSTOM_LINE_COLORS], 1.5));
                // 设置通道名称
                ui->plot->graph()->setName(QString("通道 %1").arg(channels));
                // 设置数据标点
                if(ui->checkBox_ShowDataPoint->isChecked())
                    ui->plot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, Qt::GlobalColor::gray, 5));
                // 启用曲线平滑
                ui->plot->graph()->setSmooth(true);
                if(ui->plot->legend->item(channels))
                {
                    ui->plot->legend->item(channels)->setTextColor(line_colors[channels % CUSTOM_LINE_COLORS]);
                }
                // 更新通道列表框显示
                ui->listWidget_Channels->addItem(ui->plot->graph()->name());
                ui->listWidget_Channels->item(channel)->setForeground(QBrush(line_colors[channels % CUSTOM_LINE_COLORS]));
                channels++;
            }
            ui->plot->graph(channel)->addData(dataPointNumber, newData[channel].toDouble());
            channel++;
        }
        dataPointNumber++;
    }
    // youShallNotPass = false;
}

/**
 * @brief Slot for spin box for plot minimum value on y axis
 * @param arg1
 */
void MainWindow::on_spinAxesMin_valueChanged(int arg1)
{
    // 判断是否进行拖动动作 && 正在放大 Y 轴 则不进行主动调用重绘
    if(this->bMousePressedInPlot == false   &&
       this->bShiftModifierPressedInPlot == false)
    {
        ui->plot->yAxis->setRangeLower(arg1);
        ui->plot->replot(QCustomPlot::rpQueuedReplot);  // 重绘
    }
}

/**
 * @brief Slot for spin box for plot maximum value on y axis
 * @param arg1
 */
void MainWindow::on_spinAxesMax_valueChanged(int arg1)
{
    // 判断是否进行拖动动作 && 正在放大 Y 轴 则不进行主动调用重绘
    if(this->bMousePressedInPlot == false   &&
       this->bShiftModifierPressedInPlot == false)
    {
        ui->plot->yAxis->setRangeUpper(arg1);
        ui->plot->replot(QCustomPlot::rpQueuedReplot);  // 重绘
    }
}

/**
 * @brief Read data for inside serial port
 */
void MainWindow::readData()
{
    if(this->serialInfo.serialPort->bytesAvailable())
    {                                                               // If any bytes are available
        QByteArray data = this->serialInfo.serialPort->readAll();  // Read all data in QByteArray

        if(!data.isEmpty())
        {                              // If the byte array is not empty
            char *temp = data.data();  // Get a '\0'-terminated char* to the data

            this->serialInfo.serialRxCnt += data.size();
            this->uartCntLabel->setText(QString("R: %1  |  T: %2").arg(this->serialInfo.serialRxCnt).arg(this->serialInfo.serialTxCnt));

            if(!filterDisplayedData)
            {
                ui->textEdit_UartWindow->moveCursor(QTextCursor::End);  // 光标位置移动到最后
                if(this->serialInfo.comDecodeType == Serial_Decode_Type::UTF_8)
                    ui->textEdit_UartWindow->insertPlainText(data);
                else if(this->serialInfo.comDecodeType == Serial_Decode_Type::HEX)
                    ui->textEdit_UartWindow->insertPlainText(data.toHex(' ').toUpper());
            }
            for(int i = 0; temp[i] != '\0'; i++)
            {                           // Iterate over the char*
                switch(STATE)
                {                       // Switch the current state of the message
                    case WAIT_START:    // If waiting for start [$], examine each char
                        if(temp[i] == START_MSG)
                        {  // If the char is $, change STATE to IN_MESSAGE
                            STATE = IN_MESSAGE;
                            receivedData.clear();  // Clear temporary QString that holds the message
                            break;                 // Break out of the switch
                        }
                        break;
                    case IN_MESSAGE:  // If state is IN_MESSAGE
                        if(temp[i] == END_MSG)
                        {  // If char examined is ;, switch state to END_MSG
                            STATE = WAIT_START;
                            QStringList incomingData = receivedData.split(' ');  // Split string received from port and put it into list
                            if(filterDisplayedData)
                            {
                                ui->textEdit_UartWindow->append(receivedData);
                            }
                            emit newData(incomingData);  // Emit signal for data received with the list
                            break;
                        }
                        else if(isdigit(temp[i]) || isspace(temp[i]) || temp[i] == '-' || temp[i] == '.')
                        {
                            /* If examined char is a digit, and
                             * not '$' or ';', append it to
                             * temporary string */
                            receivedData.append(temp[i]);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

/**
 * @brief Spin box for changing the Y Tick step
 * @param arg1
 */
void MainWindow::on_spinYStep_valueChanged(int arg1)
{
    ui->plot->yAxis->ticker()->setTickCount(arg1);
    ui->plot->replot();
    ui->spinYStep->setValue(ui->plot->yAxis->ticker()->tickCount());
}

/**
 * @brief Save a PNG image of the plot to current EXE directory
 */
void MainWindow::on_savePNGButton_clicked()
{
    ui->plot->savePng(QString::number(dataPointNumber) + ".png", 1920, 1080, 2, 50);
}

/**
 * @brief 数据标点选择框 状态改变事件
 * @param state Qt::CheckState
 */
void MainWindow::on_checkBox_ShowDataPoint_stateChanged(int state)
{
    if(state == Qt::CheckState::Checked)
    {
        for(int index=0; index<ui->plot->graphCount(); index++)
            ui->plot->graph(index)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, Qt::GlobalColor::gray, 5));
    }
    else
    {
        for(int index=0; index<ui->plot->graphCount(); index++)
            ui->plot->graph(index)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));
    }
    ui->plot->replot();
}

/**
 * @brief <槽函数> 图表重绘完成事件
 * @param event
 */
void MainWindow::onAfterPlot()
{
    // qDebug("after replot.");
}

/**
 * @brief <槽函数> 图表区域鼠标点击事件
 * @param event
 */
void MainWindow::onMousePressInPlot(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->bMousePressedInPlot = true;
}

/**
 * @brief <槽函数> 图表区域鼠标释放事件
 * @param event
 */
void MainWindow::onMouseReleaseInPlot(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->bMousePressedInPlot = false;
}

/**
 * @brief <槽函数> 图表鼠标移动事件
 * @param event
 */
void MainWindow::onMouseMoveInPlot(QMouseEvent *event)
{
    // 判断是否按住 Shift 键
    if(event->modifiers() != Qt::ShiftModifier)
    {
        this->bShiftModifierPressedInPlot = false;
    }

    // 显示 X Y 坐标信息
    // 将鼠标坐标值换成图表轴的值
    int xx = int(ui->plot->xAxis->pixelToCoord(event->x()));
    int yy = int(ui->plot->yAxis->pixelToCoord(event->y()));
    QString coordinates("X: %1 Y: %2");
    coordinates = coordinates.arg(xx).arg(yy);
    ui->statusBar->showMessage(coordinates);

    // 若为拖动状态 则更新文本框信息
    if(this->bMousePressedInPlot == true)
    {
        ui->spinAxesMax->setValue(int(ui->plot->yAxis->range().upper));
        ui->spinAxesMin->setValue(int(ui->plot->yAxis->range().lower));
        // 取消 X 轴自动滚动
        if(ui->checkBox_XAxisAutoScroll->isChecked())
        {
            ui->checkBox_XAxisAutoScroll->setChecked(false);
        }
    }

    // 判断图表中有无通道
    if(!ui->plot->graphCount())
        return;
    // 判断有无通道被选中
    int i = 0;
    for(i = 0; i < ui->plot->graphCount(); i++)
    {
        if(ui->plot->graph(i)->selected())
        {
            break;
        }
    }
    if(i == ui->plot->graphCount())
    {
        return;
    }
    // 绘制十字坐标
    // 获取 X 轴值对应的曲线中的 Y 轴值
    double yVal = ui->plot->graph(i)->data()->at(xx)->value;
    // 定义标签格式
    QString tip = QString::number(xx) + ", " + QString::number(yVal); // (x, y)

    // 点位用 tooltip 显示
    QToolTip::showText(cursor().pos(), tip, ui->plot);
    // 设置追踪曲线的通道
    plotTracer->setGraph(ui->plot->graph(i));
    // 按照 X 轴的值进行追踪
    plotTracer->setGraphKey(xx);
    // 更新追踪位置
    plotTracer->updatePosition();
    // 设置追踪曲线可视
    plotTracer->setVisible(true);
    // 更新图表
    // ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

/**
 * @brief <槽函数> 图表鼠标滚轮事件
 * @param event
 */
void MainWindow::onMouseWhellInPlot(QWheelEvent *event)
{
    // 判断是否按住 Shift 键
    if(event->modifiers() == Qt::ShiftModifier)
    {
        this->bShiftModifierPressedInPlot = true;               // 标志 Shift 键已在图表中按下
        ui->plot->axisRect()->setRangeZoom(Qt::Vertical);       // 轴缩放方向
        ui->plot->setInteraction(QCP::iRangeZoom, true);        // 允许原生放大
        // 调整缩放因子
        ui->plot->axisRect()->setRangeZoomFactor(0.0,
                                                 0.9);
        // ui->plot->replot(QCustomPlot::rpQueuedReplot);          // 重绘
        // 更新文本框信息
        ui->spinAxesMax->setValue(int(ui->plot->yAxis->range().upper));
        ui->spinAxesMin->setValue(int(ui->plot->yAxis->range().lower));
        return;
    }
    ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);         // 轴缩放方向
    int lastSpinVal = ui->spinPoints->value();
    this->bPlotZoomWithWheel = true;      // 标志图表缩放事件是鼠标滚轮事件发起的

    // 将鼠标滚轮事件转发给 spinPonits 控件
    QWheelEvent inverted_event = QWheelEvent(event->posF(),
                                             event->globalPosF(),
                                             -event->pixelDelta(),
                                             -event->angleDelta(),
                                             0,
                                             Qt::Vertical,
                                             event->buttons(),
                                             event->modifiers());
    QApplication::sendEvent(ui->spinPoints, &inverted_event);

    // 若当前处于暂停绘图状态 根据鼠标位置放大 X 轴
    if(this->bPlotting == false || ui->checkBox_XAxisAutoScroll->isChecked() == false)
    {
        int curSpinVal = ui->spinPoints->value();

        ui->plot->setInteraction(QCP::iRangeZoom, true);       // 允许原生放大
        // 动态调整缩放因子
        ui->plot->axisRect()->setRangeZoomFactor(static_cast<double>(           \
                                                 static_cast<double>(           \
                                                 MIN(lastSpinVal, curSpinVal))/ \
                                                 static_cast<double>(           \
                                                 MAX(lastSpinVal,curSpinVal))),
                                                 0.0);
        ui->plot->replot(QCustomPlot::rpQueuedReplot);
    }
}

/**
 * @brief <槽函数> 图表线条选择事件
 */
void MainWindow::channel_selection(void)
{
    int i = 0;
    for(i = 0; i < ui->plot->graphCount(); i++)
    {
        QCPGraph *graph = ui->plot->graph(i);
        if(graph->selected() == false)
            continue;
        QCPPlottableLegendItem *item = ui->plot->legend->itemWithPlottable(graph);
        item->setSelected(graph->selected());
        // 通道列表框选中对应的项目
        ui->listWidget_Channels->setCurrentRow(i);
    }
    if(i == ui->plot->graphCount())
    {
        plotTracer->setVisible(false);
        ui->plot->replot();
    }

}

/**
 * @brief <槽函数> 图例被单击
 * @param legend  图例
 * @param item
 * @param event
 */
void MainWindow::legend_click(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED(legend)
    Q_UNUSED(event)
    if(item == nullptr)
    {
        return;
    }
    // 选中图例 item
    item->setSelected(true);
    // 遍历 Graph 并选中 item 对应的 Graph
    for(int i=0; i<ui->plot->graphCount(); i++)
    {
        QCPGraph *graph = ui->plot->graph(i);
        QCPPlottableLegendItem *PlottableLegenditem = ui->plot->legend->itemWithPlottable(graph);
        if(PlottableLegenditem->selected())
        {
            // 全选图表对应的通道的全部数据
            graph->setSelection(QCPDataSelection(QCPDataRange(0, INT_MAX)));
            // 通道列表框选中对应的项目
            ui->listWidget_Channels->setCurrentRow(i);
        }
    }
}

/**
 * @brief <槽函数> 图例 item 双击事件
 * @param legend
 * @param item
 */
void MainWindow::legend_double_click(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED(legend)
    Q_UNUSED(event)

    /* Only react if item was clicked (user could have clicked on border
     * padding of legend where there is no item, then item is 0) */
    if(item != nullptr)
    {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem *>(item);
        bool ok;
        QString newName = QInputDialog::getText(this, "设置标题", "新标题:", QLineEdit::Normal, plItem->plottable()->name(), &ok, Qt::Popup);
        if(ok)
        {
            plItem->plottable()->setName(newName);
            for(int i = 0; i < ui->plot->graphCount(); i++)
            {
                ui->listWidget_Channels->item(i)->setText(ui->plot->graph(i)->name());
            }
            ui->plot->replot();
        }
    }
}

/**
 * @brief Spin box controls how many data points are collected and displayed
 * @param curSpinVal
 */
void MainWindow::on_spinPoints_valueChanged(int curSpinVal)
{
    // 判断当前是否正在绘图 且 X 轴自动滚动功能被打开
    if(this->bPlotting  &&
       ui->checkBox_XAxisAutoScroll->isChecked())
    {
        ui->plot->setInteraction(QCP::iRangeZoom, false);   // 不允许原生放大
        ui->plot->xAxis->setRange(dataPointNumber - ui->spinPoints->value(),
                                dataPointNumber);           // 设置 X 轴以右侧放大
    }
    else// 若当前处于暂停绘图状态 根据 X 轴中间位置放大 X 轴
    {
        // 判断事件发起者是否为处于图表内的鼠标滚轮
        if(this->bPlotZoomWithWheel)
        {
            this->bPlotZoomWithWheel = false;
            return;
        }
        // 图表 X 轴沿中点缩放
        ui->plot->xAxis->setRange(ui->plot->xAxis->range().center(),
                                  curSpinVal,
                                  Qt::AlignmentFlag::AlignCenter);
    }
    ui->plot->replot();
}

/**
 * @brief 说明按钮点击事件
 */
void MainWindow::on_actionHow_to_use_triggered()
{
    helpWindow = new HelpWindow(this);
    helpWindow->setWindowTitle("说明");
    Qt::WindowFlags flags = helpWindow->windowFlags();
    helpWindow->setWindowFlags(flags & ~Qt::WindowContextHelpButtonHint); // 去除问号
    helpWindow->set_version(QString("V " VERSION));
    helpWindow->show();
    // RTTJLinkHandle = new RTTJLink(this);
}

/**
 * @brief Connects to COM port or restarts bPlotting
 */
void MainWindow::on_actionConnect_triggered()
{
    // 判断是否已打开串口
    if(bSerialConnected)
    {
        // 若已经打开串口 则继续绘图
        if(bPlotting == false)
        {
            updateTimer.start();  // 开启绘图定时器
            bPlotting = true;
            ui->actionConnect->setEnabled(false);
            ui->actionPausePlot->setEnabled(true);
            ui->statusBar->showMessage("开始绘图");
        }
    }
    else
    {
        // 若未打开串口 则开启串口
        QSerialPortInfo portInfo(ui->comboPort->currentText().split(" ").takeAt(0));    // 获取选择框对应的串口设备的信息
        int baudRate = ui->comboBaud->currentText().toInt();                            // 获取波特率信息
        this->serialInfo.serialPort = new QSerialPort(portInfo, nullptr);               // 根据串口选择框对应的串口设备创建串口对象

        QT_TRY
        {
            // 打开串口设备
            openPort(portInfo,
                    baudRate,
                    this->serialInfo.curSerialDataBits,
                    this->serialInfo.curSerialParity,
                    this->serialInfo.curSerialStopBits);
        }
        QT_CATCH(Except ex)
        {
            if(ex == EXCEP_ZERO) QT_RETHROW;
            qDebug("error");
        }
    }
}

/**
 * @brief 暂停绘图按钮点击事件
 */
void MainWindow::on_actionPausePlot_triggered()
{
    // 若当前正在绘图 则暂停绘图
    if(bPlotting == true)
    {
        updateTimer.stop();  // 停止绘图定时器
        bPlotting = false;
        ui->actionConnect->setEnabled(true);
        ui->actionPausePlot->setEnabled(false);
        ui->statusBar->showMessage("暂停绘图");
    }
}

/**
 * @brief 记录数据按钮 状态翻转事件
 */
void MainWindow::on_actionRecord_stream_triggered()
{
    if(ui->actionRecord_stream->isChecked())
    {
        ui->statusBar->showMessage("数据将会保存为 csv 文件");
    }
    else
    {
        ui->statusBar->showMessage("停止记录数据");
    }
}

/**
 * @brief 断开连接按钮 状态翻转事件
 */
void MainWindow::on_actionDisconnect_triggered()
{
    if(bSerialConnected)
    {
        this->serialInfo.serialPort->close();   // 关闭串口
        emit portClosed();                      // Notify application
        delete this->serialInfo.serialPort;     // Delete the pointer
        this->serialInfo.serialPort = nullptr;  // Assign NULL to dangling pointer
        ui->statusBar->showMessage("连接已断开!");
        bSerialConnected = false;                      // 标志为串口已断开
        ui->actionConnect->setEnabled(true);

        bPlotting = false;  // Not bPlotting anymore
        ui->actionPausePlot->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
        ui->actionRecord_stream->setEnabled(true);
        receivedData.clear();  // Clear received string

        ui->savePNGButton->setEnabled(false);
        enable_com_controls(true);
    }
}

/**
 * @brief Clear all channels data and reset plot area
 *
 * This function will not delete the channel itself (legend will stay)
 */
void MainWindow::on_actionClear_triggered()
{
    ui->plot->clearPlottables();
    ui->listWidget_Channels->clear();
    channels = 0;
    dataPointNumber = 0;
    emit setupPlot();
    ui->plot->replot();
}

/**
 * @brief 创建或打开新的 CSV 文件以保存数据
 */
void MainWindow::openCsvFile(void)
{
    m_csvFile = new QFile(QDateTime::currentDateTime().toString("yyyy-MM-d-HH-mm-ss-") + "data-out.csv");
    if(!m_csvFile)
        return;
    if(!m_csvFile->open(QIODevice::ReadWrite | QIODevice::Text))
        return;
}

/**
 * @brief 关闭 CSV 文件
 */
void MainWindow::closeCsvFile(void)
{
    if(!m_csvFile)
        return;
    m_csvFile->close();
    if(m_csvFile)
        delete m_csvFile;
    m_csvFile = nullptr;
}

/**
 * @brief 打开 CSV 文件以保存数据
 */
void MainWindow::saveStream(QStringList newData)
{
    if(!m_csvFile)
        return;
    if(ui->actionRecord_stream->isChecked())
    {
        QTextStream out(m_csvFile);
        foreach(const QString &str, newData)
        {
            out << str << ",";
        }
        out << "\n";
    }
}

/**
 * @brief <槽函数> 隐藏文本框按钮 点击事件
 */
void MainWindow::on_pushButton_TextEditHide_clicked()
{
    if(ui->pushButton_TextEditHide->isChecked())
    {
        ui->textEdit_UartWindow->setVisible(false);
        ui->pushButton_TextEditHide->setText("显示文本框");
    }
    else
    {
        ui->textEdit_UartWindow->setVisible(true);
        ui->pushButton_TextEditHide->setText("隐藏文本框");
    }
}

/**
 * @brief <槽函数> 文本过滤按钮 点击事件
 */
void MainWindow::on_pushButton_ShowallData_clicked()
{
    if(ui->pushButton_ShowallData->isChecked())
    {
        filterDisplayedData = false;
        ui->checkBox_UartDecodeWithHex->setEnabled(true);
        ui->pushButton_ShowallData->setText("显示过滤文本");
    }
    else
    {
        filterDisplayedData = true;
        ui->checkBox_UartDecodeWithHex->setEnabled(false);
        ui->pushButton_ShowallData->setText("显示原始文本");
    }
}

/**
 * @brief <槽函数> Y 轴自动调整按钮 点击事件
 */
void MainWindow::on_pushButton_AutoScale_clicked()
{
    ui->plot->yAxis->rescale(true);
    ui->spinAxesMax->setValue(int(ui->plot->yAxis->range().upper) + int(ui->plot->yAxis->range().upper * 0.1));
    ui->spinAxesMin->setValue(int(ui->plot->yAxis->range().lower) + int(ui->plot->yAxis->range().lower * 0.1));
}

/**
 * @brief <槽函数> 重置视图按钮 点击事件
 */
void MainWindow::on_pushButton_ResetVisible_clicked()
{
    for(int i = 0; i < ui->plot->graphCount(); i++)
    {
        ui->plot->graph(i)->setVisible(true);
        ui->listWidget_Channels->item(i)->setBackground(Qt::NoBrush);
    }
}

/**
 * @brief 通道选择列表框 通道双击事件
 * @param item 被双击的项目
 */
void MainWindow::on_listWidget_Channels_itemDoubleClicked(QListWidgetItem *item)
{
    int graphIndex = ui->listWidget_Channels->currentRow();

    // 隐藏或显示图表对应通道
    if(ui->plot->graph(graphIndex)->visible())
    {
        ui->plot->graph(graphIndex)->setVisible(false);
        item->setForeground(Qt::black);
        // item->setStyleSheet("text-decoration: line-through;");
        item->setBackgroundColor(Qt::black);
    }
    else
    {
        ui->plot->graph(graphIndex)->setVisible(true);
        item->setForeground(QBrush(line_colors[graphIndex % CUSTOM_LINE_COLORS]));
        // item->setStyleSheet("text-decoration: none;");
        item->setBackground(Qt::NoBrush);
    }

    // 图表重绘
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

/**
 * @brief <槽函数> 刷新按钮 点击事件
 */
void MainWindow::on_pushButton_Refresh_clicked()
{
    ui->comboPort->clear();
    /* List all available serial ports and populate ports combo box */
//    for(QSerialPortInfo port : QSerialPortInfo::availablePorts())
//    {
//        ui->comboPort->addItem(port.portName());
//    }
//    if(ui->comboPort->count())
//    {
//        enable_com_controls(true);
//    }

    // 检查是否存在串口设备
    if(QSerialPortInfo::availablePorts().size() == 0)
    {
        enable_com_controls(false);
        ui->actionPausePlot->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);
        ui->savePNGButton->setEnabled(false);
        ui->statusBar->showMessage("未找到串口设备");
    }
    else
    {
        // 遍历串口设备并将设备信息赋值到控件
        for(QSerialPortInfo port : QSerialPortInfo::availablePorts())
        {
            ui->comboPort->addItem(port.portName() + " " + port.description());
        }
        if(this->bSerialConnected == false)
        {
            enable_com_controls(true);
        }
    }
}

/**
 * @brief <槽函数> 更多串口设置按钮 点击事件
 */
void MainWindow::on_pushButton_MoreUartSetting_clicked()
{
    serialSettingWindow = new SerialSetting(this);
    Qt::WindowFlags flags = serialSettingWindow->windowFlags();
    serialSettingWindow->setWindowFlags(flags & ~Qt::WindowContextHelpButtonHint); // 去除问号
    serialSettingWindow->set_param(this->serialInfo.curSerialDataBits,
                                   this->serialInfo.curSerialParity,
                                   this->serialInfo.curSerialStopBits);
    connect(this->serialSettingWindow,
            SIGNAL(okPressed(QSerialPort::DataBits,
                             QSerialPort::Parity,
                             QSerialPort::StopBits)),
            this,
            SLOT(serial_setting_window_handle(QSerialPort::DataBits,
                                              QSerialPort::Parity,
                                              QSerialPort::StopBits)));
    serialSettingWindow->show();
}

/**
 * @brief <槽函数> 更多串口设置窗口点击确定 事件回调函数
 */
void MainWindow::serial_setting_window_handle(QSerialPort::DataBits dataBits,
                                              QSerialPort::Parity parity,
                                              QSerialPort::StopBits stopBits)
{
    this->serialInfo.curSerialDataBits = dataBits;
    this->serialInfo.curSerialParity   = parity;
    this->serialInfo.curSerialStopBits = stopBits;
    // 判断当前是否已打开串口
    if(bSerialConnected)
    {
        this->serialInfo.serialPort->setDataBits(dataBits);
        this->serialInfo.serialPort->setParity(parity);
        this->serialInfo.serialPort->setStopBits(stopBits);
    }
}

/**
 * @brief <槽函数> 文本框清空按钮 点击事件
 */
void MainWindow::on_pushButton_ClearTextEdit_clicked()
{
    ui->textEdit_UartWindow->clear();
    this->serialInfo.serialRxCnt = 0;
    this->serialInfo.serialTxCnt = 0;
}

/**
 * @brief <槽函数> 自动滚动按钮 点击事件
 */
void MainWindow::on_pushButton_AutoScroll_clicked()
{
    ui->textEdit_UartWindow->moveCursor(QTextCursor::End);
}

/**
 * @brief <槽函数> HEX 选择框 状态改变事件
 */
void MainWindow::on_checkBox_UartDecodeWithHex_stateChanged(int state)
{
    if(state == Qt::CheckState::Checked)
    {
        this->serialInfo.comDecodeType = Serial_Decode_Type::HEX;
    }
    else
    {
        this->serialInfo.comDecodeType = Serial_Decode_Type::UTF_8;
    }
}
