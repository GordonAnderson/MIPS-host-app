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
#include <QTabWidget>

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
    void SetState(bool ShutDown);
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

class CPbutton : public QWidget
{
    Q_OBJECT
signals:
    void CPselected(QString);
public:
    CPbutton(QWidget *parent, QString name, QString CPfilename, int x, int y);
    void Show(void);
    QWidget *p;
    QString Title;
    QString FileName;
    int     X,Y;
private:
    QPushButton *pbCPselect;
private slots:
    void pbCPselectPressed(void);
};

class LightWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool on READ isOn WRITE setOn)
public:
    LightWidget(const QColor &color, QWidget *parent = NULL)
        : QWidget(parent), m_color(color), m_on(false) {}

    bool isOn() const
        { return m_on; }
    void setOn(bool on)
    {
        if (on == m_on)
            return;
        m_on = on;
        update();
    }

public slots:
    void turnOff() { setOn(false); }
    void turnOn() { setOn(true); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!m_on)
        {
            //return;
            QColor offc;
            if(m_color == Qt::red) offc = Qt::darkRed;
            if(m_color == Qt::yellow) offc = Qt::darkYellow;
            if(m_color == Qt::green) offc = Qt::darkGreen;
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(offc);
            painter.drawEllipse(0, 0, width(), height());
            return;
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(m_color);
        painter.drawEllipse(0, 0, width(), height());
    }

private:
    QColor m_color;
    bool m_on;
};

class TrafficLightWidget : public QWidget
{
public:
    TrafficLightWidget(QWidget *parent = NULL)
        : QWidget(parent)
    {
        QVBoxLayout *vbox = new QVBoxLayout(this);
        m_red = new LightWidget(Qt::red);
        vbox->addWidget(m_red);
        m_yellow = new LightWidget(Qt::yellow);
        vbox->addWidget(m_yellow);
        m_green = new LightWidget(Qt::green);
        vbox->addWidget(m_green);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::gray);
        setPalette(pal);
        setAutoFillBackground(true);
    }

    LightWidget *redLight() const
        { return m_red; }
    LightWidget *yellowLight() const
        { return m_yellow; }
    LightWidget *greenLight() const
        { return m_green; }

private:
    LightWidget *m_red;
    LightWidget *m_yellow;
    LightWidget *m_green;
};

class StatusLight : public QWidget
{
    Q_OBJECT
public:
    StatusLight(QWidget *parent, QString name, int x, int y);
    void Show(void);
    void ShowMessage(void);
    QString ProcessCommand(QString cmd);
    QWidget *p;
    QString Title;
    QString Status;
    QString Mode;
    int     X,Y;

private:
    QLabel *TL;
    QLabel *label;
    QLabel *Message;
    TrafficLightWidget *widget;
};

class ControlPanel : public QDialog
{
    Q_OBJECT

signals:
    void DialogClosed(QString NextCP);

public:
    explicit ControlPanel(QWidget *parent, QString CPfileName, QList<Comms*> S, Properties *prop);
    ~ControlPanel();
    virtual void reject();
    void Update(void);
    Q_INVOKABLE QString Save(QString Filename);
    Q_INVOKABLE QString Load(QString Filename);
    void InitMIPSsystems(QString initFilename);
    void LogDataFile(void);
    QString findFile(QString filename, QString posiblePath);
    QList<Comms*> Systems;
    QStatusBar  *statusBar;
    DCBchannel  *FindDCBchannel(QString name);
    bool LoadedConfig;
    Properties *properties;
    QList<QWidget *> Containers;
    //QWidget *Container;
    // The following functions are for the scripting system
    Q_INVOKABLE QString GetLine(QString MIPSname);
    Q_INVOKABLE bool SendCommand(QString MIPSname, QString message);
    Q_INVOKABLE QString SendMess(QString MIPSname, QString message);
    Q_INVOKABLE void SystemEnable(void);
    Q_INVOKABLE void SystemShutdown(void);
    Q_INVOKABLE bool isShutDown(void);
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
    Q_INVOKABLE bool isComms(void);
    Q_INVOKABLE bool SystemIsShutdown;  // Cannot access this from a javascript

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

    QList<QTabWidget *>    Tabs;
    QList<QGroupBox *>     GroupBoxes;
    QList<ScriptButton *>  ScripButtons;
    QList<Cpanel *>        Cpanels;
    QList<TextLabel *>     TextLabels;
    QList<RFchannel *>     RFchans;
    QList<RFCchannel *>    RFCchans;
    QList<ADCchannel *>    ADCchans;
    QList<DACchannel *>    DACchans;
    QList<DCBchannel *>    DCBchans;
    QList<DCBoffset *>     DCBoffsets;
    QList<DCBenable *>     DCBenables;
    QList<DIOchannel *>    DIOchannels;
    QList<ESI *>           ESIchans;
    QList<ARBchannel *>    ARBchans;
    QList<RFamp *>         rfa;
    QList<Ccontrol *>      Ccontrols;
    QList<QStringList *>   CSVdata;
    QList<Plot *>          plots;
    QList<Device *>        devices;
    QList<CPbutton *>      CPbuttons;
    IFTtiming              *IFT;
    ShutterTG              *shutterTG;
    QList<TimingControl *> TC;
    QList<Compressor  *>   comp;
    StatusLight *          sl;
    uint        LogStartTime;
    int         LogPeriod;
    int         UpdateHoldOff;
    int         SerialWatchDog;
    bool        UpdateStop;
    bool        ShutdownFlag;
    bool        RestoreFlag;
//  bool        SystemIsShutdown;
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
    void slotCPselected(QString);
};

#endif // CONTROLPANEL_H
