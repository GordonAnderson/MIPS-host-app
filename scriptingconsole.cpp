#include "scriptingconsole.h"
#include "ui_scriptingconsole.h"
#include "help.h"
#include <qscriptengine.h>
#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>

ScriptingConsole::ScriptingConsole(QWidget *parent, Properties *prop) :
    QDialog(parent),
    ui(new Ui::ScriptingConsole)
{
   ui->setupUi(this);

   properties = prop;

   this->setFixedSize(501,366);
   engine = new QScriptEngine(this);
   engine->setProcessEventsInterval(10);

   QScriptValue mips = engine->newQObject(parent);
   engine->globalObject().setProperty("mips",mips);
}

ScriptingConsole::~ScriptingConsole()
{
    delete ui;
}

void ScriptingConsole::paintEvent(QPaintEvent *event)
{
    UpdateStatus();
}

void ScriptingConsole::RunScript(void)
{
    if(engine->isEvaluating()) return;
    if(properties != NULL) properties->Log("Script executed");
    QString script = ui->txtScript->toPlainText();
    QScriptValue result = engine->evaluate(script);
    QString markup;
    if(result.isError())
        markup.append("<font color=\"red\">");
    markup.append(result.toString());
    if(result.isError())
        markup.append("</font>");
    ui->lblStatus->setText(markup);
}

void ScriptingConsole::on_pbEvaluate_clicked()
{
    ui->pbEvaluate->setDown(false);
    RunScript();
}

void ScriptingConsole::UpdateStatus(void)
{
    if(engine->isEvaluating())
    {
        ui->pbEvaluate->setText("Script is running!");
    }
    else
    {
        ui->pbEvaluate->setText("Execute");
    }
}

void ScriptingConsole::on_pbSave_clicked()
{
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->ScriptPath);
    QString fileName = fileDialog.getSaveFileName(this, tr("Save script to file"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        stream << ui->txtScript->toPlainText();
        file.close();
        if(properties != NULL) properties->Log("Script saved: " + fileName);
    }
}

void ScriptingConsole::on_pbLoad_clicked()
{
    QFileDialog fileDialog;

    if(properties != NULL) fileDialog.setDirectory(properties->ScriptPath);
    QString fileName = fileDialog.getOpenFileName(this, tr("Load script from File"),"",tr("scrpt (*.scrpt);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    ui->txtScript->clear();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString text = stream.readAll();
        ui->txtScript->append(text);
        file.close();
        if(properties != NULL) properties->Log("Script loaded: " + fileName);
    }
}

void ScriptingConsole::on_pbAbort_clicked()
{
    engine->abortEvaluation();
    if(properties != NULL) properties->Log("Script aborted");
}

void ScriptingConsole::on_pbHelp_clicked()
{
    Help *help = new Help();

    help->SetTitle("Script Help");
    help->LoadHelpText(":/scripthelp.txt");
    help->show();
}

// **********************************************************************************************
// ScriptButton   *******************************************************************************
// **********************************************************************************************
ScriptButton::ScriptButton(QWidget *parent, QString name, QString ScriptFile, int x, int y, Properties *prop, QStatusBar *statusbar)
{
    p      = parent;
    X      = x;
    Y      = y;
    Title = name;
    FileName = ScriptFile;
    properties = prop;
    sb = statusbar;
}

void ScriptButton::Show(void)
{
    pbButton = new QPushButton(Title,p);
    pbButton->setGeometry(X,Y,150,32);
    pbButton->setAutoDefault(false);
    connect(pbButton,SIGNAL(pressed()),this,SLOT(pbButtonPressed()),Qt::QueuedConnection);
    engine = new QScriptEngine(this);
    engine->setProcessEventsInterval(100);
    QScriptValue mips;
    if(p->parentWidget()->parentWidget() != NULL)
       mips = engine->newQObject(p->parentWidget()->parentWidget());
    else
       mips = engine->newQObject(p->parentWidget());
    engine->globalObject().setProperty("mips",mips);
}

// This event fires when button is pressed, start script and turn
// button on when pressed. If pressed when script is running ask user
// if he would like to abort script.
void ScriptButton::pbButtonPressed(void)
{
    static bool busy = false;
    QMessageBox msgBox;

    pbButton->setDown(false);
    if(busy)
    {
        msgBox.setText("A script is currently running, do you want to abort?");
        msgBox.setInformativeText("Select yes to abort");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
        engine->abortEvaluation();
        if(properties != NULL) properties->Log("Script aborted");
        busy = false;
        return;
    }
    // Load Script
    QFile file(FileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString text = stream.readAll();
        file.close();
        if(properties != NULL) properties->Log("Script loaded: " + FileName);
        busy = true;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();
        QScriptValue result = engine->evaluate(text);
        if(result.isError())
        {
            if(properties != NULL) properties->Log("Script error: " + result.toString());
            if(sb != NULL) sb->showMessage("Script error: " + result.toString());
        }
        else
        {
            if(properties != NULL) properties->Log("Script finished!");
            if(sb != NULL) sb->showMessage("Script finished!");
        }
    }
    else
    {
        if(properties != NULL) properties->Log("Can't open script file: " + FileName);
        if(sb != NULL) sb->showMessage("Can't open script file: " + FileName);
    }
    busy = false;
    QApplication::restoreOverrideCursor();
}
