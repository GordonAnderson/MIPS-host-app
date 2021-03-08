#include "shuttertg.h"
#include "ui_shuttertg.h"

#include <QDebug>
#include <QKeyEvent>

#include "qcustomplot.h"

#define ClockFreq   2625000

ShutterTG::ShutterTG(QWidget *parent, QString name, QString MIPSname) :
    QDialog(parent),
    ui(new Ui::ShutterTG)
{
    ui->setupUi(this);

    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    comms  = NULL;
    sb     = NULL;

    ui->gbShutterTG->setTitle(Title);

    // Init the pulse sequence variables and controls
    Tp = 150;
    Shutters[0].Name = "A";
    Shutters[1].Name = "B";
    Shutters[2].Name = "C";
    Shutters[3].Name = "D";
    Shutters[4].Name = "F";    // ADC trigger
    for(int i=0; i<MaxShutters; i++)
    {
        Shutters[i].Width = 50;
        Shutters[i].Delay = 100;
        Shutters[i].Open = 0;
        Shutters[i].Close = 1;
    }
    Shutters[0].Delay = Shutters[4].Delay = 0;
    Shutters[4].Open = 1;
    Shutters[4].Close = 0;
    n = 10;
    for(int i=0; i<MaxScanPoints; i++)
    {
      Tdn[i] = (i+1) * 5;
      Repeat[i] = 5;
    }
    SeqRepeat = 10;
    ui->comboSelectedShutter->clear();
    ui->comboSelectedShutter->addItem("B");
    ui->comboSelectedShutter->addItem("C");
    ui->comboSelectedShutter->addItem("D");
    PulseSeqVarUpdate();

    connect(ui->leTp,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTsA,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTsB,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTsC,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTsD,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTdB,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTdC,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTdD,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leN,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leTdn,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leM,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->leFullRepeat,SIGNAL(editingFinished()),this,SLOT(PulseSeqVarChange()));
    connect(ui->comboSelectN,SIGNAL(currentIndexChanged(int)),this,SLOT(PulseSeqNselect()));
    connect(ui->pbDownload,SIGNAL(pressed()),this,SLOT(Download()));
    connect(ui->pbShow,SIGNAL(pressed()),this,SLOT(ShowTable()));
    connect(ui->pbStart,SIGNAL(pressed()),this,SLOT(StartTable()));
    connect(ui->pbDefScan,SIGNAL(pressed()),this,SLOT(slotDefineScan()));

    clipboard = QApplication::clipboard();
    connect(ui->pbPaste,SIGNAL(pressed()),this,SLOT(slotDataChanged()));
}

ShutterTG::~ShutterTG()
{
    // Make sure system is in local mode
    if(comms != NULL) comms->SendString("SMOD,LOC\n");
    delete ui;
}

void ShutterTG::slotDataChanged(void)
{
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasText())
    {
        // Text data
        QString res = mimeData->text();
        QStringList reslist = res.split(QRegExp("[\r\n\t ,]+"), QString::SkipEmptyParts);
        // The count should be a multople of three
        ui->leN->setText(QString::number((int)(reslist.count()/3)));
        ui->leN->setModified(true);
        ui->leN->editingFinished();
        QApplication::processEvents();
        for(int i=0;i<(int)(reslist.count());i+=3)
        {
           if(reslist[i].toInt() != i/3+1) break;
           ui->comboSelectN->setCurrentIndex(i/3);
           QApplication::processEvents();
           ui->leTdn->setText(reslist[i+1]);
           ui->leTdn->setModified(true);
           ui->leTdn->editingFinished();
           QApplication::processEvents();
           ui->leM->setText(reslist[i+2]);
           ui->leM->setModified(true);
           ui->leM->editingFinished();
           QApplication::processEvents();
        }
    }
}

void ShutterTG::PulseSeqVarUpdate(void)
{
    ui->leTp->setText(QString::number(Tp));
    ui->leTsA->setText(QString::number(Shutters[0].Width));
    ui->leTsB->setText(QString::number(Shutters[1].Width));
    ui->leTsC->setText(QString::number(Shutters[2].Width));
    ui->leTsD->setText(QString::number(Shutters[3].Width));
    ui->leTdB->setText(QString::number(Shutters[1].Delay));
    ui->leTdC->setText(QString::number(Shutters[2].Delay));
    ui->leTdD->setText(QString::number(Shutters[3].Delay));
    ui->leN->setText(QString::number(n));
    ui->comboSelectN->clear();
    for(int i=0; i<n; i++) ui->comboSelectN->addItem(QString::number(i+1));
    ui->leTdn->setText(QString::number(Tdn[0]));
    ui->leM->setText(QString::number(Repeat[0]));
    ui->leFullRepeat->setText(QString::number(SeqRepeat));
}

void ShutterTG::PulseSeqVarChange(void)
{
    Tp = ui->leTp->text().toFloat();
    Shutters[0].Width = ui->leTsA->text().toFloat();
    Shutters[1].Width = ui->leTsB->text().toFloat();
    Shutters[2].Width = ui->leTsC->text().toFloat();
    Shutters[3].Width = ui->leTsD->text().toFloat();
    Shutters[1].Delay = ui->leTdB->text().toFloat();
    Shutters[2].Delay = ui->leTdC->text().toFloat();
    Shutters[3].Delay = ui->leTdD->text().toFloat();
    n = ui->leN->text().toInt();
    // Range testing
    if(n > MaxScanPoints) ui->leN->setText(QString::number(n = MaxScanPoints));
    if(n < 1) ui->leN->setText(QString::number(n = 1));
    if((Shutters[0].Width < 20) && (Shutters[0].Width != 0))ui->leTsA->setText(QString::number(Shutters[0].Width = 20));
    if((Shutters[1].Width < 20) && (Shutters[1].Width != 0)) ui->leTsB->setText(QString::number(Shutters[1].Width = 20));
    if((Shutters[2].Width < 20) && (Shutters[2].Width != 0)) ui->leTsC->setText(QString::number(Shutters[2].Width = 20));
    if((Shutters[3].Width < 20) && (Shutters[3].Width != 0)) ui->leTsD->setText(QString::number(Shutters[3].Width = 20));
    if(Tp < 1) ui->leTp->setText(QString::number(Tp = 1));
    Tdn[ui->comboSelectN->currentIndex()] = ui->leTdn->text().toFloat();
    Repeat[ui->comboSelectN->currentIndex()] = ui->leM->text().toFloat();
    SeqRepeat = ui->leFullRepeat->text().toInt();
    if(n != ui->comboSelectN->count())
    {
        ui->comboSelectN->clear();
        for(int i=0; i<n; i++)
        {
            ui->comboSelectN->addItem(QString::number(i+1));
        }
        ui->leTdn->setText(QString::number(Tdn[0]));
        ui->leM->setText(QString::number(Repeat[0]));
    }
}

void ShutterTG::PulseSeqNselect(void)
{
    ui->leTdn->setText(QString::number(Tdn[ui->comboSelectN->currentIndex()]));
    ui->leM->setText(QString::number(Repeat[ui->comboSelectN->currentIndex()]));
}

void ShutterTG::Download(void)
{
    QString res;

    ui->pbDownload->setDown(false);
    if(comms == NULL) return;
    // Make sure system is in local mode
    comms->SendString("SMOD,LOC\n");
    comms->msDelay(2000);
    // Enable tasks in table mode
    comms->SendCommand("TBLTSKENA,TRUE\n");
    // Set clock
    comms->SendCommand("STBLCLK," + QString::number(ClockFreq) + "\n");
    // Set trigger
    comms->SendCommand("STBLTRG,POS\n");
    // Send table
    comms->SendCommand(GenerateTable());
    // Put system in table mode
    comms->SendCommand("SMOD,TBL\n");
    comms->waitforline(100);
    sb->showMessage(comms->getline());
}

// This function returns the next event time in seconds that happens
// after the current time defined by lastTime
double ShutterTG::GetEventNextTime(double lastTime)
{
    double nextTime = -1;
    double dT, wT;

    for(int i=0;i<MaxShutters;i++)
    {
       dT = Shutters[i].Delay / 1000;
       wT = dT + Shutters[i].Width / 1000000;
       if(Shutters[i].Width > 0)
       {
           if(nextTime == -1)
           {
               if(dT > lastTime) nextTime = dT;
               else if(wT > lastTime) nextTime = wT;
           }
           else
           {
               if(dT > lastTime) if(dT < nextTime) nextTime = dT;
               if(wT > lastTime) if(wT < nextTime) nextTime = wT;
           }
       }
    }
    return nextTime;
}

QString ShutterTG::GetEventsAtTime(double timePoint)
{
    double dT, wT;
    QString events;

    events.clear();
    for(int i=0;i<MaxShutters;i++)
    {
       if(Shutters[i].Width > 0)
       {
           dT = Shutters[i].Delay / 1000;
           wT = dT + Shutters[i].Width / 1000000;
           if(dT == timePoint) events += ":" + Shutters[i].Name + ":" + QString::number(Shutters[i].Open);
           if(wT == timePoint)  events += ":" + Shutters[i].Name + ":" + QString::number(Shutters[i].Close);
       }
    }
    return events;
}


// MIPS digital outputs assignments:
//  A = Shutter A
//  B = Shutter B
//  C = Shutter C
//  D = Shutter D
//  E = Sync output
//  F = A, ADC trigger
//
// Note! Logic 1 = shutter closed
QString ShutterTG::GenerateTable(void)
{
   QString TableName;
   Shutter *SelectedShutter = NULL;
   float   SavedDelay;

   PulseSeqVarChange();
   TableName = "A";
   // Initial table with sync pulse
   Table = "STBLDAT;0:[L:" + QString::number((int)(SeqRepeat)) + "," + QString::number((int)(50 * ClockFreq / 1000000)) + ":E:1";
   for(int i=0;i<4;i++) if(Shutters[i].Width == 0) Table += ":" + Shutters[i].Name + ":" + QString::number(Shutters[i].Open);
   Table += ",";
   Table += QString::number((int)(100 * ClockFreq / 1000000))  + ":E:0:";
   // Get the selected shutter used for time sweeping in the sequence
   for(int i=0;i<4;i++) if(Shutters[i].Name == ui->comboSelectedShutter->currentText()) SelectedShutter = &Shutters[i];
   if(SelectedShutter == NULL) return ("");
   SavedDelay = SelectedShutter->Delay;
   // If A with is 0 then set F width to 50, else set F width to A width
   if(Shutters[0].Width == 0) Shutters[4].Width = 50;
   else Shutters[4].Width = Shutters[0].Width;
   // Add each sequence to the table
   for(int i=0; i<n; i++)
   {
       SelectedShutter->Delay = Tdn[i];
       if(i > 0) Table += ",0:";
       Table += "[" + TableName + ":" + QString::number(Repeat[i]);
       double lastTime = -1;
       while(true)
       {
           lastTime = GetEventNextTime(lastTime);
           if(lastTime < 0) break;
           Table +=  "," + QString::number((int)(lastTime * ClockFreq)) + GetEventsAtTime(lastTime);
       }
       Table += "," + QString::number((int)(Tp * ClockFreq / 1000)) + ":]";
       TableName = TableName[0].unicode() + 1;
   }
   // Now Terminate the table command and its ready to send to MIPS!
   Table += "," + QString::number((int)(50 * ClockFreq / 1000000)) + ":];";
   SelectedShutter->Delay = SavedDelay;
   return Table;
}

void ShutterTG::ShowTable(void)
{
    QString table;

    ui->pbShow->setDown(false);
    table = GenerateTable();
    //qDebug() << table.length();
    QMessageBox msgBox;
    msgBox.setText(table);
    msgBox.exec();
}

void ShutterTG::StartTable(void)
{
    if(comms == NULL) return;
    ui->pbStart->setDown(false);
    comms->SendCommand("TBLSTRT\n");
    comms->waitforline(100);
    sb->showMessage(comms->getline());
    comms->waitforline(500);
    sb->showMessage(sb->currentMessage() + " " + comms->getline());
    comms->waitforline(100);
    sb->showMessage(sb->currentMessage() + " " + comms->getline());
}

QString ShutterTG::Report(void)
{
    QString res;
    QStringList resList;
    QString title;

    PulseSeqVarChange();
    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res.clear();
    QObjectList widgetList = ui->gbShutterTG->children();
    widgetList += ui->gbSequenceRepeats->children();
    foreach(QObject *w, widgetList)
    {
        //qDebug() << w->objectName();
        if(w->objectName().startsWith("le"))
        {
            res += title + "," + w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
        }
    }
    res += title + ",Tdn";
    for(int i=0; i<n; i++) res += "," + QString::number(Tdn[i]);
    res += "\n";
    res += title + ",Repeat";
    for(int i=0; i<n; i++) res += "," + QString::number(Repeat[i]);
    res += "\n";
    return res;
}

bool ShutterTG::SetValues(QString strVals)
{
    QStringList resList,ctrlList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 3) return false;
    QObjectList widgetList = ui->gbShutterTG->children();
    widgetList += ui->gbSequenceRepeats->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().startsWith(resList[1]))
        {
            if(w->objectName().startsWith("le"))
            {
                ((QLineEdit *)w)->setText(resList[2]);
                ((QLineEdit *)w)->setModified(true);
                ((QLineEdit *)w)->editingFinished();
                PulseSeqVarChange();
                return true;
            }
        }
    }
    if(resList[1] == "Tdn")
    {
        for(int i=0; i<resList.count() - 2; i++) Tdn[i] = resList[i+2].toFloat();
        PulseSeqVarChange();
        return true;
    }
    if(resList[1] == "Repeat")
    {
        for(int i=0; i<resList.count() - 2; i++) Repeat[i] = resList[i+2].toInt();
        PulseSeqVarChange();
        return true;
    }
    return false;
}

void ShutterTG::slotDefineScan(void)
{
    bool ok;
    QString text;

    ui->pbDefScan->setDown(false);
    // Pop message describing fution and asking for user confirmation
    QMessageBox msgBox;
    QString msg = "This function will generate a series of sequences to sweep a shutter pulse. ";
    msg += "You will define start and stop times, number of steps and averages at each step. ";
    msg += "Note, the start time for the sweep must be after all fuxed shutter times.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to contine?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    while(true)
    {
        text = QInputDialog::getText(0, "Define scan", "Enter start time, mS:", QLineEdit::Normal,"", &ok );
        if(!ok) break;
        float startT = text.toFloat();
        text = QInputDialog::getText(0, "Define scan", "Enter stop time, mS:", QLineEdit::Normal,"", &ok );
        if(!ok) break;
        float stopT = text.toFloat();
        text = QInputDialog::getText(0, "Define scan", "Enter number of steps:", QLineEdit::Normal,"", &ok );
        if(!ok) break;
        float steps = text.toInt();
        text = QInputDialog::getText(0, "Define scan", "Averages at each time point:", QLineEdit::Normal,"", &ok );
        if(!ok) break;
        float averages = text.toInt();
        // Error testing
        if((stopT <= startT) || (steps <= 1) || (steps > 2000) || (averages <= 0) || (startT <= 0) || (stopT <= 0))
        {
            msg = "Parameter error! Make sure stop time is greater that start time, steps is greater ";
            msg += "one and that averages is greater than 0.";
            msgBox.setText(msg);
            msgBox.setInformativeText("");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
        // Calculate all the steps and populate the controls
        float delta = (stopT - startT) / (steps-1);
        for(int i=0;i<steps;i++)
        {
            Tdn[i] = startT + delta * i;
            Repeat[i] = averages;
        }
        n = steps;
        Tp = stopT + delta;
        PulseSeqVarUpdate();
        break;
    }
}
