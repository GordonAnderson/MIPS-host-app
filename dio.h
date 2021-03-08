#ifndef DIO_H
#define DIO_H

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

class DIO: public QDialog
{
    Q_OBJECT

public:
    DIO(Ui::MIPS *w, Comms *c);
    void Update(void);
    void Save(QString Filename);
    void Load(QString Filename);

    Ui::MIPS *dui;
    Comms *comms;

private slots:
    void UpdateDIO(void);
    void DOUpdated(void);
    void TrigHigh(void);
    void TrigLow(void);
    void TrigPulse(void);
    void RFgenerate(void);
    void SetFreq(void);
    void SetWidth(void);
    // Following are for remote navication of UI
    void RemoteNavigation(void);
    void RemoteNavSmallUP(void);
    void RemoteNavLargeUP(void);
    void RemoteNavSmallDown(void);
    void RemoteNavLargeDown(void);
    void RemoteNavSelect(void);
};

class DIOchannel : public QWidget
{
    Q_OBJECT
public:
    DIOchannel(QWidget *parent, QString name, QString MIPSname, int x, int y);
    void Show(void);
    void Update(void);
    QString Report(void);
    QString ProcessCommand(QString cmd);
    bool SetValues(QString strVals);
    QWidget *p;
    QString Title;
    int     X,Y;
    QString MIPSnm;
    QString Channel;
    Comms   *comms;
    bool    ReadOnly;
private:
    QFrame      *frmDIO;
    QCheckBox   *DIO;
private slots:
    void DIOChange(bool);
};

#endif // DIO_H
