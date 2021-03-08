#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include "comms.h"
#include "mipscomms.h"
#include "cmdlineapp.h"
#include "arbwaveformedit.h"
#include "script.h"
#include "help.h"
#include "rfamp.h"
#include "tcpserver.h"
#include "arb.h"
#include "adc.h"
#include "dio.h"
#include "dcbias.h"
#include "rfdriver.h"
#include "timinggenerator.h"
#include "compressor.h"
#include "properties.h"
#include "scriptingconsole.h"
#include "plot.h"
#include "device.h"
#include "shuttertg.h"

#include <QDialog>
#include <QDebug>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QProcess>
#include <QKeyEvent>
#include <QBoxLayout>
#include <QUdpSocket>
#include <QMenu>
#include <QScriptEngine>

extern QString Version;

namespace Ui {
class ControlPanel;
}

class ScriptingConsole;

class TextLabel : public QWidget
{
    Q_OBJECT
public:
    TextLabel(QWidget *parent, QString name, int size, int x, int y);
    void Show(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    int     Size;
private:
    QLabel      *label;
};

class Shutdown : public QWidget
{
    Q_OBJECT
signals:
    void ShutdownSystem(void);
    void EnableSystem(void);
public:
    Shutdown(QWidget *parent, QString name, int x, int y);
    void Show(void);
    QWidget *p;
    QString Title;
    int     X,Y;
private:
    QPushButton *pbShutdown;
private slots:
    void pbPressed(void);
};

class SaveLoad : public QWidget
{
    Q_OBJECT
signals:
    void Save(void);
    void Load(void);
public:
    SaveLoad(QWidget *parent, QString nameSave, QString nameLoad, int x, int y);
    void Show(void);
    QWidget *p;
    QString TitleSave;
    QString TitleLoad;
    int     X,Y;
private:
    QPushButton *pbSave;
    QPushButton *pbLoad;
private slots:
    void pbSavePressed(void);
    void pbLoadPressed(void);
};

class DACchannel : public QWidget
{
    Q_OBJECT
public:
    DACchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QString ProcessCommand(QString cmd);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    float   b,m;
    QString Units;
    QString Format;
private:
    QFrame      *frmDAC;
    QLineEdit   *Vdac;
    QLabel      *labels[2];
    bool         Updating;
    bool         UpdateOff;
private slots:
    void VdacChange(void);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class DCbiasGroupsEventFilter : public QObject
{
     Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class DCBiasGroups : public QWidget
{
    Q_OBJECT
signals:
    void disable(void);
    void enable(void);
public:
    DCBiasGroups(QWidget *parent, int x, int y);
    void Show(void);
    QWidget *p;
    int     X,Y;
    QComboBox   *comboGroups;
    bool SetValues(QString strVals);
    QString Report(void);
private:
    QGroupBox   *gbDCBgroups;
    QCheckBox   *DCBenaGroups;
private slots:
    void slotEnableChange(void);
};

class ESI : public QWidget
{
    Q_OBJECT
public:
    ESI(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    QString ProcessCommand(QString cmd);
    bool SetValues(QString strVals);
    void Shutdown(void);
    void Restore(void);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    int     Channel;
    Comms   *comms;
    bool    isShutdown;
private:
    QFrame      *frmESI;
    QLineEdit   *ESIsp;
    QLineEdit   *ESIrb;
    QCheckBox   *ESIena;
    QLabel      *labels[2];
    bool        activeEnableState;
private slots:
    void ESIChange(void);
    void ESIenaChange(void);
};

class Ccontrol : public QWidget
{
    Q_OBJECT
public:
    Ccontrol(QWidget *parent, QString name, QString MIPSname,QString Type, QString Gcmd, QString Scmd, QString RBcmd, QString Units, int x, int y);
    void    Show(void);
    void    Update(void);
    QString Report(void);
    bool    SetValues(QString strVals);
    void    SetList(QString strOptions);
    void    Shutdown(void);
    void    Restore(void);
    QString ProcessCommand(QString cmd);
    QWidget          *p;
    QString          Title;
    int              X,Y;
    QString          MIPSnm;
    QString          Ctype;
    QString          Dtype;
    QString          GetCmd;
    QString          SetCmd;
    QString          ReadbackCmd;
    QString          UnitsText;
    QString          ActiveValue;
    QString          ShutdownValue;
    Comms            *comms;
    bool             isShutdown;
private:
    QFrame                *frmCc;
    QLineEdit             *Vsp;
    QLineEdit             *Vrb;
    QPushButton           *pbButton;
    QCheckBox             *chkBox;
    QComboBox             *comboBox;
    QLabel                *labels[2];
    bool                  Updating;
    bool                  UpdateOff;
private slots:
    void VspChange(void);
    void pbButtonPressed(void);
    void chkBoxStateChanged(int);
    void comboBoxChanged(QString);
//protected:
//    bool eventFilter(QObject *obj, QEvent *event);
};

// Creates a new control panel and places a button on the parent control
// panel that will show this new control panel.
class Cpanel : public QWidget
{
    Q_OBJECT

public:
    explicit Cpanel(QWidget *parent, QString name, QString CPfileName, int x, int y, QList<Comms*> S, Properties *prop);
//    ~Cpanel();
    void             Show(void);    // This will show the button, pressing the button will show the control panel
    void             Update(void);
    QString          Title;
    int              X,Y;
    QWidget          *p;
    ControlPanel     *cp;
private:
    QPushButton      *pbButton;
private slots:
    void pbButtonPressed(void);
    void slotDialogClosed(void);
};

class ControlPanel : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(void);

public:
    explicit ControlPanel(QWidget *parent, QString CPfileName, QList<Comms*> S, Properties *prop);
    ~ControlPanel();
    virtual void reject();
    void Update(void);
    Q_INVOKABLE QString Save(QString Filename);
    Q_INVOKABLE QString Load(QString Filename);
    void InitMIPSsystems(QString initFilename);
    void LogDataFile(void);
    QList<Comms*> Systems;
    QStatusBar  *statusBar;
    DCBchannel  *FindDCBchannel(QString name);
    bool LoadedConfig;
    Properties *properties;
    QWidget *Container;
    // The following functions are for the scripting system
    Q_INVOKABLE QString GetLine(QString MIPSname);
    Q_INVOKABLE bool SendCommand(QString MIPSname, QString message);
    Q_INVOKABLE QString SendMess(QString MIPSname, QString message);
    Q_INVOKABLE void SystemEnable(void);
    Q_INVOKABLE void SystemShutdown(void);
    Q_INVOKABLE void Acquire(QString filePath);
    Q_INVOKABLE bool isAcquiring(void);
    Q_INVOKABLE void DismissAcquire(void);
    Q_INVOKABLE void msDelay(int ms);
    Q_INVOKABLE void statusMessage(QString message);
    Q_INVOKABLE void popupMessage(QString message);
    Q_INVOKABLE bool popupYesNoMessage(QString message);
    Q_INVOKABLE QString popupUserInput(QString title, QString message);
    Q_INVOKABLE bool UpdateHalted(bool stop);
    Q_INVOKABLE void WaitForUpdate(void);
    Q_INVOKABLE QString SelectFile(QString fType, QString Title, QString Ext);
    Q_INVOKABLE int ReadCSVfile(QString fileName, QString delimiter);
    Q_INVOKABLE QString ReadCSVentry(int line, int entry);
    Q_INVOKABLE QString Command(QString cmd);
    Q_INVOKABLE void CreatePlot(QString Title, QString Yaxis, QString Xaxis, int NumPlots);
    Q_INVOKABLE void PlotCommand(QString cmd);

private:
    Comms            *FindCommPort(QString name, QList<Comms*> Systems);
    Ui::ControlPanel *ui;
    TCPserver *tcp;
    QMenu   *contextMenu2Dplot;
    QAction *GeneralHelp;
    QAction *MIPScommands;
    QAction *ScriptHelp;
    QAction *ThisHelp;
    QAction *OpenLogFile;
    QAction *CloseLogFile;
    QString HelpFile;
    QString LogFile;
    QString ControlPanelFile;
    QString MIPSnames;

    QList<QGroupBox *>    GroupBoxes;
    QList<ScriptButton *> ScripButtons;
    QList<Cpanel *>       Cpanels;
    QList<TextLabel *>    TextLabels;
    QList<RFchannel *>    RFchans;
    QList<RFCchannel *>   RFCchans;
    QList<ADCchannel *>   ADCchans;
    QList<DACchannel *>   DACchans;
    QList<DCBchannel *>   DCBchans;
    QList<DCBoffset *>    DCBoffsets;
    QList<DCBenable *>    DCBenables;
    QList<DIOchannel *>   DIOchannels;
    QList<ESI *>          ESIchans;
    QList<ARBchannel *>   ARBchans;
    QList<RFamp *>        rfa;
    QList<Ccontrol *>     Ccontrols;
    QList<QStringList *>  CSVdata;
    QList<Plot *>         plots;
    QList<Device *>       devices;
    IFTtiming             *IFT;
    ShutterTG             *shutterTG;
    QList<TimingControl *> TC;
    QList<Compressor  *>   comp;
    uint        LogStartTime;
    int         LogPeriod;
    int         UpdateHoldOff;
    int         SerialWatchDog;
    bool        UpdateStop;
    bool        ShutdownFlag;
    bool        RestoreFlag;
    bool        SystemIsShutdown;
    bool        StartMIPScomms;
    bool        RequestUpdate;
    Script      *script;
    Shutdown    *SD;
    SaveLoad    *SL;
    MIPScomms   *mc;
    DCBiasGroups        *DCBgroups;
    QPushButton         *MIPScommsButton;
    QList<QPushButton *> ARBcompressorButton;
    QPushButton         *scriptButton;
    ScriptingConsole    *scriptconsole;
    QUdpSocket          *udpSocket;
    Help                *help;
public slots:
    void pbSD(void);
    void pbSE(void);
    void pbSave(void);
    void pbLoad(void);
    void pbMIPScomms(void);
    void CloseMIPScomms(void);
    void scriptShutdown(void);
    void scriptLoad(QString Filename);
    void DCBgroupDisable(void);
    void DCBgroupEnable(void);
    void slotDataAcquired(QString filepath);
    void slotDataFileDefined(QString filepath);
    void pbScript(void);
    void popupHelp(QPoint);
    void slotGeneralHelp(void);
    void slotMIPScommands(void);
    void slotScriptHelp(void);
    void slotThisControlPanelHelp(void);
    void tcpCommand(void);
    void pbARBcompressor(void);
    void slotLogStatusBarMessage(QString);
    void slotOpenLogFile(void);
    void slotCloseLogFile(void);
};

#endif // CONTROLPANEL_H
