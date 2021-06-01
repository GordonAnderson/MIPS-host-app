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
    engine = NULL;
    CallOnUpdate = false;
    CallOnStart = false;
    ScriptText.clear();
    busy = false;
    //engine = new QScriptEngine(p);
}

ScriptButton::~ScriptButton()
{
    if(engine != NULL)
    {
        engine->abortEvaluation();
    }
}

void ScriptButton::Show(void)
{
    QWidget          *parent;

    pbButton = new QPushButton(Title,p);
    pbButton->setGeometry(X,Y,150,32);
    pbButton->setAutoDefault(false);
    connect(pbButton,SIGNAL(pressed()),this,SLOT(pbButtonPressed()),Qt::QueuedConnection);
    engine = new QScriptEngine(this);
    engine->setProcessEventsInterval(100);
    parent = p->parentWidget();
    while(parent->parentWidget() != NULL) parent = parent->parentWidget();
    mips = engine->newQObject(parent);
    engine->globalObject().setProperty("mips",mips);
}

// This event fires when button is pressed, start script and turn
// button on when pressed. If pressed when script is running ask user
// if he would like to abort script.
void ScriptButton::pbButtonPressed(void)
{
    ButtonPressed(true);
}

void ScriptButton::ButtonPressed(bool AlwaysLoad)
{
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
        // Make sure the halted flag is cleared, call the mips property
        QScriptValueList arg;
        QScriptValue v = false;
        arg.append(v);
        mips.property("UpdateHalted").call(mips,arg);
        return;
    }
    // Load Script if needed
    if((AlwaysLoad) || (ScriptText.isEmpty()))
    {
        QFile file(FileName);
        if(file.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            if(properties != NULL) properties->Log("Script loaded: " + FileName);
            QTextStream stream(&file);
            ScriptText = stream.readAll();
            file.close();
        }
        else
        {
            if(properties != NULL) properties->Log("Can't open script file: " + FileName);
            if(sb != NULL) sb->showMessage("Can't open script file: " + FileName);
            return;
        }
    }
    if(!ScriptText.isEmpty())
    {
        busy = true;
        if(AlwaysLoad) QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();
        QScriptValue result = engine->evaluate(ScriptText);
        if(AlwaysLoad)
        {
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
    }
    busy = false;
    if(AlwaysLoad) QApplication::restoreOverrideCursor();
}

QString ScriptButton::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(title == cmd.trimmed())
    {
        ButtonPressed(false);
        return("");
    }
    else return("?");
}

void ScriptButton::Update(void)
{
    if(CallOnUpdate) ButtonPressed(false);
    if(CallOnStart)
    {
        QMessageBox *msg = new QMessageBox();
        msg->setText("Configuring system, standby...");
        msg->setStandardButtons(0);
        msg->show();
        CallOnStart = false;
        ButtonPressed(false);
        msg->hide();
    }
}

