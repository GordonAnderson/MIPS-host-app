#ifndef TIMINGGENERATOR_H
#define TIMINGGENERATOR_H

#include "comms.h"
#include "cmdlineapp.h"
#include "dcbias.h"
#include "cdirselectiondlg.h"
#include "properties.h"

#include <QDialog>
#include <QStatusBar>
#include <QProcess>
#include <QGroupBox>
#include <QInputDialog>

bool DownloadTable(Comms *comms, QString Table, QString ClockSource, QString TriggerSource);
QString MakePathUnique(QString path);

typedef struct
{
    QString Name;
    QString Channel;
    QString Start;
    QString Width;
    float   StartT;
    float   WidthT;
    float   Value;
    float   ValueOff;
} Event;

class AcquireData : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
    void dataFileDefined(QString filePath);
public:
    AcquireData(QWidget *parent = 0);
    void        StartAcquire(QString path, int FrameSize, int Accumulations);
    bool        isRunning(void);
    void        Dismiss(void);
    Comms       *comms;
    QWidget     *p;
    QStatusBar  *statusBar;
    QString     filePath;
    QString     fileName;
    QString     TriggerMode;
    bool        TableDownloaded;
    bool        Acquiring;
    QString     Acquire;
    Properties  *properties;
    int         LastFrameSize;
    int         LastAccumulations;

private:
    cmdlineapp   *cla;

private slots:
    void slotAppReady(void);
    void slotAppFinished(void);
    void slotDialogClosed(void);
};

namespace Ui {
class TimingGenerator;
}

class AcquireData;

class EventControl : public QWidget
{
    Q_OBJECT
signals:
    void EventChanged(QString Ename, QString Val);

public:
    EventControl(QWidget *parent, QString name, QString Ename, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    bool SetValues(QString strVals);
    QString ProcessCommand(QString cmd);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString ECname;
    Comms   *comms;
    QString Gcommand;
    QString Scommand;
    QLineEdit   *EventValue;
private:
    QFrame      *frmEvent;
    QLabel      *label;
private slots:
    void EventChange(void);
};

class TimingGenerator : public QDialog
{
    Q_OBJECT

public:
    explicit TimingGenerator(QWidget *parent, QString name, QString MIPSname);
    ~TimingGenerator();
    void    AddSignal(QString Title, QString Chan);
    void    UpdateEvents(void);
    QString Report(void);
    bool    SetValues(QString strVals);
    QString ProcessCommand(QString cmd);
    QString GenerateMuxSeq(QString Seq);
    bool    isTableMode(void);
    int     ConvertToCount(QString val);
    QStringList Split(QString str, QString del);
    QWidget     *p;
    QString     Title;
    QString     MIPSnm;
    Comms       *comms;
    QStatusBar  *statusBar;
    Event       *selectedEvent;
    QList<Event *>        Events;
    QList<EventControl *> EC;
    Ui::TimingGenerator *ui;
    QPushButton   *Edit;
    QPushButton   *Trigger;
    QPushButton   *Abort;
    Properties    *properties;
    int           FrameCtAdj;

public slots:
    void slotEventChange(void);
    void slotEventUpdated(void);
    void slotGenerate(void);
    void slotClearEvents(void);
    void slotLoad(void);
    void slotSave(void);
    void slotClearTable(void);
};

class TimingControl : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
    void dataFileDefined(QString filePath);
public:
    TimingControl(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void              Show(void);
    void              AcquireData(QString path);
    class AcquireData *AD;
    QWidget           *p;
    QString           Title;
    QString           Acquire;
    int               X,Y;
    QString           MIPSnm;
    QString           filePath;
    QString           fileName;
    Comms             *comms;
    QStatusBar        *statusBar;
    QProcess          process;
    TimingGenerator   *TG;
    bool              TableDownloaded;
    bool              Acquiring;
    Properties       *properties;
    bool              Downloading;
    bool              AlwaysGenerate;
    int               FrameCtAdj;

private:
    QGroupBox        *gbTC;
    QPushButton      *Edit;
    QPushButton      *Trigger;
    QPushButton      *Abort;
//    cmdlineapp       *cla;

public slots:
    void pbEdit(void);
    void pbTrigger(void);
    void pbAbort(void);
    void slotDataAcquired(QString);
    void slotDataFileDefined(QString);
    void slotEventChanged(QString, QString);
};

class IFTtiming : public QWidget
{
    Q_OBJECT
signals:
    void dataAcquired(QString filePath);
public:
    IFTtiming(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void        Show(void);
    QString     Report(void);
    bool        SetValues(QString strVals);
    void        AcquireData(QString path);
    QWidget     *p;
    class AcquireData *AD;
    QString     Title;
    int         X,Y;
    QString     MIPSnm;
    QString     filePath;
    Comms       *comms;
    QString     Enable;
    QStatusBar  *statusBar;
    DCBchannel  *Grid1;
    DCBchannel  *Grid2;
    DCBchannel  *Grid3;
    QProcess    process;
    bool        TableDownloaded;
    Properties  *properties;

private:
    QGroupBox    *gbIFT;
    QLineEdit    *leFillTime;
    QLineEdit    *leTrapTime;
    QLineEdit    *leReleaseTime;
    QLineEdit    *leGrid1FillV;
    QLineEdit    *leGrid2ReleaseV;
    QLineEdit    *leGrid3ReleaseV;
    QLineEdit    *Accumulations;
    QLineEdit    *FrameSize;
    QLineEdit    *Table;
    QComboBox    *ClockSource;
    QComboBox    *TriggerSource;
    QPushButton  *GenerateTable;
    QPushButton  *Download;
    QPushButton  *Trigger;
    QPushButton  *Abort;
    QLabel       *labels[11];
public slots:
    void slotDataAcquired(QString);
    void pbGenerate(void);
    void pbDownload(void);
    void pbTrigger(void);
    void pbAbort(void);
    void tblObsolite(void);
};

#endif // TIMINGGENERATOR_H
