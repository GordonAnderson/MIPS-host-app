#ifndef SHUTTERTG_H
#define SHUTTERTG_H

#include "comms.h"

#include <QDialog>
#include <QStatusBar>

#define MaxShutters 5
#define MaxScanPoints 2000

namespace Ui {
class ShutterTG;
}

typedef struct
{
    QString Name;
    float   Width;
    float   Delay;
    int     Open;
    int     Close;
} Shutter;

class ShutterTG : public QDialog
{
    Q_OBJECT

public:
    explicit ShutterTG(QWidget *parent, QString name, QString MIPSname);
    ~ShutterTG();
    QString Report(void);
    bool SetValues(QString strVals);
    QWidget     *p;
    QString     Title;
    QString     MIPSnm;
    QStatusBar  *sb;
    Comms       *comms;

private:
    void PulseSeqVarUpdate(void);
    double GetEventNextTime(double lastTime);
    QString GetEventsAtTime(double timePoint);
    QString GenerateTable(void);
    Ui::ShutterTG *ui;
    // Pulse sequence variables
    float Tp;
    Shutter Shutters[MaxShutters];
    Shutter *SortedShutters[MaxShutters];
    int   n;
    float Tdn[MaxScanPoints];
    int   Repeat[MaxScanPoints];
    int   SeqRepeat;
    QString Table;
    const QClipboard *clipboard;

private slots:
    void slotDataChanged(void);
    void PulseSeqVarChange(void);
    void PulseSeqNselect(void);
    void Download(void);
    void ShowTable(void);
    void StartTable(void);
    void slotDefineScan(void);
};

#endif // SHUTTERTG_H
