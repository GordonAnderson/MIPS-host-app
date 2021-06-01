#ifndef SCRIPTINGCONSOLE_H
#define SCRIPTINGCONSOLE_H

#include <QDialog>
#include <QScriptEngine>
#include <QStatusBar>

#include "properties.h"

namespace Ui {
class ScriptingConsole;
}

class ScriptingConsole : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptingConsole(QWidget *parent = 0, Properties *prop = NULL);
    void UpdateStatus(void);
    void RunScript(void);
    ~ScriptingConsole();
    Properties  *properties;

protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::ScriptingConsole *ui;
    QScriptEngine *engine;

private slots:
    void on_pbEvaluate_clicked();
    void on_pbSave_clicked();
    void on_pbLoad_clicked();
    void on_pbAbort_clicked();
    void on_pbHelp_clicked();
};

// Creates a script buttom on the control panel. When pressed the
// script is loaded and executed. If a script is alreay executing
// then the user is asked if he would like to abort.
class ScriptButton : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptButton(QWidget *parent, QString name, QString ScriptFile, int x, int y, Properties *prop, QStatusBar *statusbar);
    ~ScriptButton();
    void             Show(void);
    void             Update(void);
    QString          ProcessCommand(QString cmd);
    QString          Title;
    QString          FileName;
    int              X,Y;
    QWidget          *p;
    QStatusBar       *sb;
    Properties       *properties;
    QString          ScriptText;
    bool             CallOnUpdate;
    bool             CallOnStart;
    bool             busy;
private:
    QScriptValue     mips;
    QPushButton      *pbButton;
    QScriptEngine    *engine;
    void ButtonPressed(bool AlwaysLoad);
private slots:
    void pbButtonPressed(void);
};

#endif // SCRIPTINGCONSOLE_H
