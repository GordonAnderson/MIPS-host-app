/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "console.h"

#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <qdatetime.h>
#include <QtCore/QDebug>
#include <QTextCodec>

Console::Console(QWidget *parent,Ui::MIPS *w, Comms *c) : QPlainTextEdit(parent) , localEchoEnabled(false)
{
    cui = w;
    comms = c;

//  document()->setMaximumBlockCount(100);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::yellow);
    setPalette(p);
}

void Console::resize(QWidget *parent)
{
    setFixedWidth(parent->width());
    setFixedHeight(parent->height());
}

void Console::putData(const QByteArray &data)
{
    insertPlainText(QString(data));

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void Console::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void Console::keyPressEvent(QKeyEvent *e)
{
    QClipboard *clipboard = QApplication::clipboard();
    QByteArray bksp;

   switch (e->key())
   {
        case Qt::Key_Backspace:
          bksp.append(8);
          getData(bksp);
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            QPlainTextEdit::keyPressEvent(e);
            break;
        case Qt::Key_V:
            if(e->modifiers() & 0x4000000)
            {
                // Here if paste key combo is pressed
                QByteArray data = clipboard->text().toStdString().c_str();
                if (localEchoEnabled) QPlainTextEdit::appendPlainText(clipboard->text());
                emit getData(clipboard->text().toStdString().c_str());
                break;
            }
        default:
            if (localEchoEnabled) QPlainTextEdit::keyPressEvent(e);
            emit getData(e->text().toLocal8Bit());
    }
}

void Console::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    setFocus();
}

void Console::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
}

void Console::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
}

void Console::Save(QString Filename)
{
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Terminal data, " + dateTime.toString() + "\n";
        stream <<  document()->toPlainText();
        //qDebug() <<  document()->toPlainText();
    }
    file.close();
    cui->statusBar->showMessage("Data saved to " + Filename,2000);
}

void Console::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
            QByteArray encodedString = codec->fromUnicode(line + "\n");
            putData(encodedString);
            if(line.trimmed().mid(0,1) != "#") emit getData(encodedString);
            QApplication::processEvents();
        } while(!line.isNull());
        file.close();
        cui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}
