#include "cmdlineapp.h"
#include "ui_cmdlineapp.h"

cmdlineapp::cmdlineapp(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cmdlineapp)
{
    ui->setupUi(this);
    ui->plot->hide();
    ReadyMessage = "";
    InputRequest = "";
    ContinueMessage = "";
    fileName = "";
    responseTimer = new QTimer;
    messageTimer = new QTimer;
    messages.clear();
    process.close();

    cmdlineapp::setWindowTitle("Acquire");
    connect(messageTimer, SIGNAL(timeout()), this, SLOT(messageProcessor()));
    connect(responseTimer, SIGNAL(timeout()), this, SLOT(sendNO()));
    connect(&process,SIGNAL(readyReadStandardOutput()),this,SLOT(readProcessOutput()));
    connect(&process,SIGNAL(readyReadStandardError()),this,SLOT(readProcessOutput()));
    connect(&process,SIGNAL(finished(int)),this,SLOT(AppFinished()));
    connect(ui->leCommand,SIGNAL(editingFinished()),this,SLOT(readMessage()));
}

cmdlineapp::~cmdlineapp()
{
    delete ui;
}

void cmdlineapp::reject()
{
    process.close();
    emit DialogClosed();
    delete this;
}

void cmdlineapp::Dismiss()
{
    process.close();
    emit DialogClosed();
    delete this;
}

void cmdlineapp::AppendText(QString message)
{
    ui->txtTerm->appendPlainText(message);
}

void cmdlineapp::Clear(void)
{
   cmdlineapp::setWindowTitle("Acquire");
   ui->txtTerm->clear();
}

void cmdlineapp::Execute(void)
{
    QStringList arguments;

    cmdlineapp::setWindowTitle("Acquire, " + appPath);
    if(process.isOpen())
    {
        // Here if the process is still running, so its waiting for a Y
        // to continue. Then it will wait for a filename with no options.
        // "Enter file name: "
        // Send Y
        ui->txtTerm->appendPlainText("Acquire loop is restarting...\n");
        // Messages to send to acquire app to cause a restart
        messages.append("Y");
        messages.append(appPathNoOptions);
        messageTimer->start(2000);
        return;
    }
    arguments << "-c";
    arguments << appPath;
#if defined(Q_OS_MAC)
    process.start("/bin/bash",arguments);
#else
    process.start(appPath);
#endif
    ui->txtTerm->appendPlainText("Application should start soon...\n");
    process.waitForStarted(-1);
}

void cmdlineapp::setupPlot(QString mess)
{
    QStringList reslist = mess.split(",");
    if(reslist.count() != 5) return;
    // Setup the plot used to display the waaveform
    ui->plot->clearGraphs();
    ui->plot->addGraph();
    if( ui->plot->plotLayout()->rowCount() > 1)
    {
        ui->plot->plotLayout()->removeAt(ui->plot->plotLayout()->rowColToIndex(0, 0));
        ui->plot->plotLayout()->simplify();
    }
    ui->plot->plotLayout()->insertRow(0);
    ui->plot->plotLayout()->addElement(0, 0, new QCPTextElement(ui->plot, reslist[1], QFont("sans", 12, QFont::Bold)));
    // give the axes some labels:
    ui->plot->xAxis->setLabel(reslist[3]);
    ui->plot->yAxis->setLabel(reslist[2]);
    // set axes ranges, so we see all data:
    ui->plot->xAxis->setRange(0, reslist[4].toInt());
    ui->plot->yAxis->setRange(0, 100);
    ui->plot->replot();
    ui->plot->show();
}

void cmdlineapp::plotDataPoint(QString mess)
{
    QStringList reslist = mess.split(",");
    if(ui->plot->isHidden()) return;
    if(reslist.count()==2)
    {
        ui->plot->graph(0)->addData(reslist[0].toDouble(), reslist[1].toDouble());
        ui->plot->yAxis->rescale(true);
        ui->plot->replot();
    }
}

void cmdlineapp::sendString(QString mess)
{
    process.write((char*)mess.data());
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (mess);
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->leCommand->setText("");
}

void cmdlineapp::sendNO(void)
{
    sendString("N\n");
    responseTimer->stop();
}

void cmdlineapp::readProcessOutput(void)
{
    QString mess;
    static QString messline="";
    static bool ploting = false;

    mess = process.readAllStandardOutput();
    if(ReadyMessage != "") if(mess.contains(ReadyMessage)) emit Ready();
    if(ContinueMessage != "") if(mess.contains(ContinueMessage))
    {
        // If here then the acquire app signaled it finished the acquire and
        // its ready to start another acquire
        AcquireFinishing();
        emit AppCompleted();
    }
    if(InputRequest != "")
    {
        if(mess.contains(InputRequest))
        {
            // Start reply timer
            responseTimer->start(5000);
        }
    }
    messline += mess;
    if(messline.contains("\n"))
    {
        QStringList messlinelist;
        messlinelist = messline.split("\n");
        messline = "";
        for(int i=1;i<messlinelist.count();i++) messline += messlinelist[i];
        for(int i=0; i<messlinelist.count();i++)
        {
            // Here with full lines of text
            if(ploting) plotDataPoint(messlinelist[i]);
            if(messlinelist[i].contains("Plot,"))
            {
                setupPlot(messlinelist[i]);
                ploting = true;
            }
        }
    }
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (mess);
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (process.readAllStandardError());
    ui->txtTerm->moveCursor (QTextCursor::End);
}

void cmdlineapp::readMessage(void)
{
    responseTimer->stop();
    process.write(ui->leCommand->text().toStdString().c_str());
    process.write("\n");
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->txtTerm->insertPlainText (ui->leCommand->text() + "\n");
    ui->txtTerm->moveCursor (QTextCursor::End);
    ui->leCommand->setText("");
}

void cmdlineapp::AcquireFinishing(void)
{
    // If filename has been defined save all the data
    // in txtTerm to the file
    if(fileName != "")
    {
        QFile file(fileName);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            // We're going to streaming text to the file
            QTextStream stream(&file);
            QDateTime dateTime = QDateTime::currentDateTime();
            stream << ui->txtTerm->toPlainText();
            file.close();
        }
    }
    ui->plot->hide();
    ui->txtTerm->appendPlainText("App signaled its finished!\n");
}

void cmdlineapp::AppFinished(void)
{
    AcquireFinishing();
    process.close();
    emit AppCompleted();
}

void cmdlineapp::messageProcessor(void)
{
    if(messages.count() != 0)
    {
        //sendString(messages[0]);
        ui->leCommand->setText(messages[0]);
        readMessage();
        messages.removeAt(0);
    }
    else messageTimer->stop();
}
