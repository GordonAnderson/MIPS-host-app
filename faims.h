#ifndef FAIMS_H
#define FAIMS_H

#include "ui_mips.h"
#include "mips.h"
#include "comms.h"

#include <QtCore/QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QStatusBar>
#include <QMessageBox>
#include <QObject>
#include <QApplication>
#include <QFileInfo>
#include <QFileDialog>

// States
#define WAITING 1
#define SCANNING 2

class FAIMS: public QDialog
{
    Q_OBJECT

public:
    FAIMS(Ui::MIPS *w, Comms *c);
    void Update(void);
    void PollLoop(void);
    bool GetNextTarget(float et);
    void Save(QString Filename);
    void Load(QString Filename);
    int getHeaderIndex(QString Name);
    QString getCSVtoken(QString Name, int index);
    void Log(QString Message);
    QString LogFileName;

    Ui::MIPS *fui;
    Comms *comms;
    bool CVparkingTriggered;
    bool WaitingForLinearScanTrig;
    bool WaitingForStepScanTrig;
    QStringList Parks;
    int   CurrentPoint;
    QString TargetCompound;
    int State;
    float TargetRT;
    float TargetWindow;
    float TargetCV;
    float TargetBias;
    // Target file variables
    QString Header;
    QStringList Records;

private slots:
    void FAIMSUpdated(void);
    void FAIMSenable(void);
    void FAIMSscan(void);
    void FAIMSloadCSV(void);
    void FAIMSstartLinearScan(void);
    void FAIMSabortLinearScan(void);
    void FAIMSstartStepScan(void);
    void FAIMSabortStepScan(void);
    void FAIMSlockOff(void);
    void FAIMSlockOn(void);
    void FAIMSselectLogFile(void);
    void slotLinearTrigOut(void);
    void slotStepTrigOut(void);
    void slotFAIMSautoTune(void);
    void slotFAIMSautoTuneAbort(void);
};

#endif // FAIMS_H
