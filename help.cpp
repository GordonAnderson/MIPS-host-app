#include "help.h"
#include "ui_help.h"
#include <QTextOption>
#include <QDir>

Help::Help(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Help)
{
    ui->setupUi(this);

    QFont font = ui->plainTextEdit->font();
    font.setFamily("Courier");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    #if defined(Q_OS_MAC)
    font.setPointSize(14);
    #else
    font.setPointSize(11);
    #endif
    QFontMetrics metrics(font);
    ui->plainTextEdit->setTabStopWidth(4 * metrics.width('X'));
    ui->plainTextEdit->setFont(font);
}

Help::~Help()
{
    delete ui;
}

void Help::SetTitle(QString title)
{
    QWidget::setWindowTitle (title);
}

void Help::LoadStr(QString DisplayText)
{
    ui->plainTextEdit->setPlainText(DisplayText);
}

void Help::LoadHelpText(QString FileName)
{
    QStringList resList;

    ui->plainTextEdit->clear();
//    QScrollBar *sb = ui->plainTextEdit->verticalScrollBar();
//    QFontMetrics metrics(ui->plainTextEdit->font());
//    ui->plainTextEdit->setTabStopWidth(tabStop * metrics.width(' '));
    //ui->plainTextEdit->setWordWrapMode(QTextOption::WrapMod NoWrap);
    ui->plainTextEdit->setWordWrapMode(QTextOption::NoWrap);
    if(FileName == "") return;
    QFile file(FileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            ui->plainTextEdit->appendPlainText(line);
        } while(!line.isNull());
        file.close();
    }
    ui->plainTextEdit->moveCursor (QTextCursor::Start);
}

void Help::resizeEvent(QResizeEvent* event)
{
   ui->plainTextEdit->setFixedWidth(Help::width());
//   #if defined(Q_OS_MAC)
    ui->plainTextEdit->setFixedHeight(Help::height()); //-(ui->statusBar->height()));
//   #else
    // Not sure why I need this 3x for a windows system??
//    ui->tabMIPS->setFixedHeight(MIPS::height()-(ui->statusBar->height()*3));
//   #endif
    //ui->plainTextEdit->setVerticalScrollBar();   setValue(0);
//    ui->plainTextEdit->setVerticalScrollBar();
//    QScrollBar *sb = ui->plainTextEdit->verticalScrollBar();
}
