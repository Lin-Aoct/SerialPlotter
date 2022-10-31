/*
 * @Date: 2022-07-28 15:54:49
 * @LastEditors: Lin-Aoct mclouds.top@foxmail.com
 * @LastEditTime: 2022-10-27 11:26:27
 * @FilePath: \code\helpwindow.cpp
 */
/***************************************************************************
**  This file is part of Serial Port Plotter                              **
**                                                                        **
**                                                                        **
**  Serial Port Plotter is a program for plotting integer data from       **
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

#include "helpwindow.hpp"
#include "ui_helpwindow.h"
#include "mainwindow.hpp"

HelpWindow::HelpWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
}

HelpWindow::~HelpWindow()
{
    delete ui;
}

/**
 * @brief 设置版本号信息
 * @param str <QString> 版本号文本
 */
void HelpWindow::set_version(QString str)
{
    ui->label_Version->setText(str);
}
