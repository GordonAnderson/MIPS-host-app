#ifndef MIPS_H
#define MIPS_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtCore/QtGlobal>
#include <QLineEdit>
#include <QTimer>
#include <QProcess>
#include <QtNetwork/QTcpSocket>
#include <QStatusBar>


namespace Ui {
class MIPS;
}

class Console;
class SettingsDialog;
class pseDialog;
class psgPoint;
class Comms;
class Twave;
class DCbias;
class DIO;
class RFdriver;
class PSG;
class Program;
class Help;
class ARB;
class FAIMS;
class SingleFunnel;
class SoftLanding;
class SoftLanding2;
class Filament;
class Grid;
class ARBwaveformEdit;
class ADC;
class ControlPanel;
class ScriptingConsole;
class Properties;

class DeleteHighlightedItemWhenShiftDelPressedEventFilter : public QObject
{
     Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class MIPS : public QMainWindow
{
    Q_OBJECT

public:
    explicit MIPS(QWidget *parent = 0);
    ~MIPS();
    virtual void closeEvent(QCloseEvent *event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mousePressEvent(QMouseEvent * event);
    Ui::MIPS *ui;
    QString SelectedTab;
    Q_INVOKABLE void RemoveTab(QString TabName);
    Q_INVOKABLE void AddTab(QString TabName);
    Q_INVOKABLE void FindMIPSandConnect(void);
    Q_INVOKABLE void FindAllMIPSsystems(void);
    Comms* FindCommPort(QString name, QList<Comms*> Systems);
    // The following functions are for the scripting system
    Q_INVOKABLE bool SendCommand(QString message);
    Q_INVOKABLE QString SendMess(QString message);
    Q_INVOKABLE bool SendCommand(QString MIPSname, QString message);
    Q_INVOKABLE QString SendMess(QString MIPSname, QString message);
    Q_INVOKABLE void msDelay(int ms);
    Q_INVOKABLE void statusMessage(QString message);
    Q_INVOKABLE void popupMessage(QString message);
    Q_INVOKABLE bool popupYesNoMessage(QString message);
    Q_INVOKABLE QString popupUserInput(QString title, QString message);

public slots:
    void setWidgets(QWidget*, QWidget*);
    void MIPSconnect(void);
    void MIPSsearch(void);
    void MIPSsetup(void);
    void MIPSdisconnect(void);
    void tabSelected();
    void writeData(const QByteArray &data);
    void readData2Console(void);
    void pollLoop(void);
    void loadSettings(void);
    void saveSettings(void);
    void DisplayAboutMessage(void);
    void clearConsole(void);
    void MIPScommands(void);
    void GeneralHelp(void);
    void Single_Funnel(void);
    void CloseSingleFunnel(void);
    void Soft_Landing(void);
    void Soft_Landing_Purdue(void);
    void CloseSoftLanding(void);
    void CloseSoftLanding2(void);
    void Grid_system(void);
    void CloseGridSystem(void);
    void GetRepeatMessage(void);
    void UpdateSystem(void);
    void GetFileFromMIPS(void);
    void PutFiletoMIPS(void);
    void ReadEEPROM(void);
    void WriteEEPROM(void);
    void ReadARBFLASH(void);
    void WriteARBFLASH(void);
    void ARBupload(void);
    void SelectCP(QString fileName = "");
    void CloseControlPanel(QString);
    void slotScripting(void);
    void DisplayProperties(void);
    void slotLogStatusBarMessage(QString);

private:
    Properties *properties;
    Comms *comms;
    Twave *twave;
    DCbias *dcbias;
    DIO *dio;
    RFdriver *rfdriver;
    PSG *SeqGen;
    Program *pgm;
    Console *console;
    SettingsDialog *settings;
    Help *help;
    ARB *arb;
    FAIMS *faims;
    Filament *filament;
    SingleFunnel *sf;
    bool sf_deleteRequest;
    SoftLanding *sl;
    bool sl_deleteRequest;
    SoftLanding2 *sl2;
    bool sl2_deleteRequest;
    Grid *grid;
    bool grid_deleteRequest;
    ControlPanel *cp;
    bool cp_deleteRequest;
    ADC *adc;
    QTimer *pollTimer;
    QString  appPath;
    QString RepeatMessage;
    QList<Comms*> Systems;
    ScriptingConsole *scriptconsole;
    QString NextCP;

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // MIPS_H
