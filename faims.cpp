#include "faims.h"
#include <QElapsedTimer>

QElapsedTimer eTimer;

int key;

FAIMS::FAIMS(Ui::MIPS *w, Comms *c)
{
    fui = w;
    comms = c;

    fui->comboFMtrig->clear();
    fui->comboFMtrig->addItem("None");
    fui->comboFMtrig->addItem("Q");
    fui->comboFMtrig->addItem("R");
    fui->comboFMtrig->addItem("S");
    fui->comboFMtrig->addItem("T");
    fui->comboFMlinearTrig->clear();
    fui->comboFMlinearTrig->addItem("None");
    fui->comboFMlinearTrig->addItem("Q");
    fui->comboFMlinearTrig->addItem("R");
    fui->comboFMlinearTrig->addItem("S");
    fui->comboFMlinearTrig->addItem("T");
    fui->comboFMstepTrig->clear();
    fui->comboFMstepTrig->addItem("None");
    fui->comboFMstepTrig->addItem("Q");
    fui->comboFMstepTrig->addItem("R");
    fui->comboFMstepTrig->addItem("S");
    fui->comboFMstepTrig->addItem("T");
    fui->comboFMlinearTrigOut->clear();
    fui->comboFMlinearTrigOut->addItem("None");
    fui->comboFMlinearTrigOut->addItem("B");
    fui->comboFMlinearTrigOut->addItem("C");
    fui->comboFMlinearTrigOut->addItem("D");
    fui->comboFMlinearTrigOut->addItem("B Active low");
    fui->comboFMlinearTrigOut->addItem("C Active low");
    fui->comboFMlinearTrigOut->addItem("D Active low");
    fui->comboFMstepTrigOut->clear();
    fui->comboFMstepTrigOut->addItem("None");
    fui->comboFMstepTrigOut->addItem("B");
    fui->comboFMstepTrigOut->addItem("C");
    fui->comboFMstepTrigOut->addItem("D");
    fui->comboFMstepTrigOut->addItem("B Active low");
    fui->comboFMstepTrigOut->addItem("C Active low");
    fui->comboFMstepTrigOut->addItem("D Active low");

    CVparkingTriggered = false;
    WaitingForLinearScanTrig = false;
    WaitingForStepScanTrig = false;
    LogFileName = "";
    QObjectList widgetList = fui->gbFAIMS_DC->children();
    widgetList += fui->gbFAIMS_RF->children();
    widgetList += fui->gbLinearScan->children();
    widgetList += fui->gbStepScan->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("leS"))
       {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(FAIMSUpdated()));
       }
    }
    connect(fui->chkFMenable,SIGNAL(toggled(bool)),this,SLOT(FAIMSenable()));
    connect(fui->chkCurtianEna,SIGNAL(toggled(bool)),this,SLOT(slotFAIMSCurtianEna()));
    connect(fui->chkCurtianIndEna,SIGNAL(toggled(bool)),this,SLOT(slotFAIMSCurtianInd()));
    connect(fui->chkNegTune,SIGNAL(toggled(bool)),this,SLOT(slotFAIMSnegTune()));
    connect(fui->pbFMstart,SIGNAL(pressed()),this,SLOT(FAIMSscan()));
    connect(fui->pbFMloadCSV,SIGNAL(pressed()),this,SLOT(FAIMSloadCSV()));
    connect(fui->pbFMstartLinear,SIGNAL(pressed()),this,SLOT(FAIMSstartLinearScan()));
    connect(fui->pbFMabortLinear,SIGNAL(pressed()),this,SLOT(FAIMSabortLinearScan()));
    connect(fui->pbFMstartStep,SIGNAL(pressed()),this,SLOT(FAIMSstartStepScan()));
    connect(fui->pbFMabortStep,SIGNAL(pressed()),this,SLOT(FAIMSabortStepScan()));
    connect(fui->pbFAIMSautoTune,SIGNAL(pressed()),this,SLOT(slotFAIMSautoTune()));
    connect(fui->pbFAIMSautoTuneAbort,SIGNAL(pressed()),this,SLOT(slotFAIMSautoTuneAbort()));
    connect(fui->rbSFMLOCK_FALSE,SIGNAL(clicked(bool)),this,SLOT(FAIMSlockOff()));
    connect(fui->rbSFMLOCK_TRUE,SIGNAL(clicked(bool)),this,SLOT(FAIMSlockOn()));
    connect(fui->pbSelectLogFile,SIGNAL(pressed()),this,SLOT(FAIMSselectLogFile()));
    connect(fui->comboFMlinearTrigOut,SIGNAL(currentTextChanged(QString)),this,SLOT(slotLinearTrigOut()));
    connect(fui->comboFMstepTrigOut,SIGNAL(currentTextChanged(QString)),this,SLOT(slotStepTrigOut()));
    eTimer.start();
}

void FAIMS::Save(QString Filename)
{

}

void FAIMS::Load(QString Filename)
{

}

// This function uses the major and minor version data from the comm object to
// enable and disable features on this interface.
void FAIMS::SetVersionOptions(void)
{
    bool state = false;
    QString res;

    if(comms == NULL) return;
    if((comms->major > 1) || (comms->minor >= 201)) state = true;
    // Auto tune options
    fui->pbFAIMSautoTune->setEnabled(state);
    fui->pbFAIMSautoTuneAbort->setEnabled(state);
    fui->chkNegTune->setEnabled(state);
    fui->lblTuneState->setEnabled(state);
    fui->leGFMTSTAT->setEnabled(state);
    // MIPS command FMISCUR, returns true if curtian supply detected in system.
    // If state is true issue FMISCUR and is reply is true the enable the
    // curtian options.
    res = comms->SendMess("FMISCUR\n");
    if(res == "TRUE") state = true;
    else state = false;
    fui->lblCurtianV->setEnabled(state);
    fui->leSHV_1->setEnabled(state);
    fui->leGHVV_1->setEnabled(state);
    fui->chkCurtianEna->setEnabled(state);
    fui->chkCurtianIndEna->setEnabled(state);
}

void FAIMS::Update(void)
{
    QString res;

    SetVersionOptions();
    QObjectList widgetList = fui->gbFAIMS_RF->children();
    widgetList += fui->gbFAIMS_DC->children();
    if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Linear scan") widgetList += fui->gbLinearScan->children();
    if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Step scan") widgetList += fui->gbStepScan->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
            if((!((QLineEdit *)w)->hasFocus()) && (((QLineEdit *)w)->isEnabled()))
            {
               res = "G" + w->objectName().mid(3).replace("_",",");
               if(res.endsWith(",")) res = res.left(res.length()-1);
               res += "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
            }
       }
    }
    res = comms->SendMess("GFMENA\n");
    if(res == "TRUE") fui->chkFMenable->setChecked(true);
    if(res == "FALSE") fui->chkFMenable->setChecked(false);
    if(fui->chkCurtianEna->isEnabled())
    {
        res = comms->SendMess("GFMENA\n");
        if(res == "TRUE") fui->chkCurtianEna->setChecked(true);
        if(res == "FALSE") fui->chkCurtianEna->setChecked(false);
    }
    if(fui->chkCurtianIndEna->isEnabled())
    {
        res = comms->SendMess("GHVSTATUS,1\n");
        if(res == "ON") fui->chkCurtianIndEna->setChecked(false);
        if(res == "OFF") fui->chkCurtianIndEna->setChecked(true);
    }
    if(fui->chkNegTune->isEnabled())
    {
        res = comms->SendMess("GFMTPOS\n");
        if(res == "TRUE") fui->chkCurtianIndEna->setChecked(false);
        if(res == "FALSE") fui->chkCurtianIndEna->setChecked(true);
    }
    // Read the lock mode and update the UI
    res = comms->SendMess("GFMLOCK\n");
    if(res == "TRUE")
    {
        fui->rbSFMLOCK_TRUE->setChecked(true);
        fui->leSFMSP->setEnabled(true);
        fui->leSFMDRV->setEnabled(false);
    }
    else if(res == "FALSE")
    {
        fui->rbSFMLOCK_FALSE->setChecked(true);
        fui->leSFMSP->setEnabled(false);
        fui->leSFMDRV->setEnabled(true);
    }
    if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Linear scan")
    {
        res = comms->SendMess("GFMSTRTLIN\n");
        if(res == "TRUE")
        {
            fui->pbFMabortLinear->setEnabled(true);
            fui->pbFMstartLinear->setEnabled(false);
        }
        else if(res == "FALSE")
        {
            fui->pbFMabortLinear->setEnabled(false);
            fui->pbFMstartLinear->setEnabled(true);
        }
    }
    if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Step scan")
    {
        res = comms->SendMess("GFMSTRTSTP\n");
        if(res == "TRUE")
        {
            fui->pbFMabortStep->setEnabled(true);
            fui->pbFMstartStep->setEnabled(false);
        }
        else if(res == "FALSE")
        {
            fui->pbFMabortStep->setEnabled(false);
            fui->pbFMstartStep->setEnabled(true);
        }
    }
}

void FAIMS::FAIMSUpdated(void)
{
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2).replace("_",",");
   if(res.endsWith(",")) res = res.left(res.length()-1);
   res += "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void FAIMS::FAIMSenable(void)
{
    if(fui->chkFMenable->isChecked()) comms->SendCommand("SFMENA,TRUE\n");
    else  comms->SendCommand("SFMENA,FALSE\n");
}

void FAIMS::FAIMSscan(void)
{
   fui->leFMcompound->setText("");
   fui->leFMcvVolts->setText("");
   fui->leFMbiasVolts->setText("");
   if((CVparkingTriggered) || (fui->pbFMstart->text() == "Stop"))
   {
       CVparkingTriggered = false;
       fui->pbFMstart->setText("Start");
       fui->leFMstatus->setText("Stopped");
   }
   else
   {
       // Turn off all trigger reporting, just in case it was on
       comms->SendCommand("RPT,Q,OFF\n");
       comms->SendCommand("RPT,R,OFF\n");
       comms->SendCommand("RPT,S,OFF\n");
       comms->SendCommand("RPT,T,OFF\n");
       fui->pbFMstart->setText("Stop");
       if(fui->comboFMtrig->currentText() == "None")
       {
           CVparkingTriggered = true;
           fui->leFMstatus->setText("Running");
           eTimer.restart();
       }
       else
       {
           // Issue the report command to MIPS for the selected channel
           // and set status to waiting for trigger
           fui->leFMstatus->setText("Waiting for trigger");
           comms->SendCommand("RPT," + fui->comboFMtrig->currentText() + ",RISING\n");
       }
       CurrentPoint = 0;
       if(!GetNextTarget(0))
       {
           CVparkingTriggered = false;
           fui->pbFMstart->setText("Start");
           fui->leFMstatus->setText("Finished");
       }
   }
}

int FAIMS::getHeaderIndex(QString Name)
{
    QStringList recordList;

    QStringList resList = Header.split(",");
    for(int i=0;i<resList.count();i++)
    {
        if(resList.at(i).startsWith(Name)) return(i);
    }
   return(-1);
}

QString FAIMS::getCSVtoken(QString Name, int index)
{
    QStringList recordList;

    QStringList resList = Header.split(",");
    for(int i=0;i<resList.count();i++)
    {
        if(resList.at(i).startsWith(Name))
        {
            recordList = Records.at(index).split(",");
            if(recordList.count() > i) return(recordList.at(i));
        }
    }
   return("");
}

bool startTime(const QString v1, const QString v2)
 {
    QStringList v1List;
    QStringList v2List;

    v1List = v1.split(",");
    v2List = v2.split(",");
    return (v1List.at(key).toFloat()-v1List.at(key+1).toFloat()/2) < (v2List.at(key).toFloat()-v2List.at(key+1).toFloat()/2);
 }

void FAIMS::FAIMSloadCSV(void)
{
    QStringList resList;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load CSV target file"),"",tr("CSV (*.csv);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);

        QString line;
        Header = stream.readLine();     // Read the CSV header, assume its the first line in the file
        Records.clear();
        do
        {
            line = stream.readLine();
            if(line != "") Records.append(line);
        } while(!line.isNull());
        file.close();
        // Sort the records by the start time, start time = RT - (RT windows) / 2
        key = getHeaderIndex("Retention Time");
        qSort(Records.begin(),Records.end(),startTime);
        // For each string in record report the following from the CVS file:
        // Compound, Retention Time, RT Window, Compensation Voltage, Bias
        for(int i=0;i<Records.count();i++)
        {
            QString rec = getCSVtoken("Compound",i) + ",";
            rec += getCSVtoken("Retention Time",i) + ",";
            rec += getCSVtoken("RT Window",i) + ",";
            rec += getCSVtoken("Compensation Voltage",i) + ",";
            rec += getCSVtoken("Bias",i);
            fui->textCVparkList->append(rec);
        }
        fui->statusBar->showMessage("CSV target file read " + fileName,2000);
    }
}

bool FAIMS::GetNextTarget(float et)
{
    QStringList entries;

    if(CurrentPoint == 0)
    {
        QString text = fui->textCVparkList->toPlainText();
        Parks = text.split( "\n" );
    }
    while(1)
    {
        while(1)
        {
            if((CurrentPoint+1) > Parks.count()) return false;
            entries = Parks.at(CurrentPoint).split(",");
            if(entries.count() == 5) break;
            CurrentPoint++;
        }
        if((entries.at(1).toFloat() + entries.at(2).toFloat()/2) > et)
        {
            TargetCompound = entries.at(0);
            TargetRT = entries.at(1).toFloat();
            TargetWindow = entries.at(2).toFloat();
            TargetCV = entries.at(3).toFloat();
            TargetBias = entries.at(4).toFloat();
            CurrentPoint++;
            State = WAITING;
            return true;
        }
        else CurrentPoint++;
    }
}

void FAIMS::PollLoop(void)
{
   float CurrentMin;
   QString res;
   QStringList reslist;

   if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "CV parking")
   {
       if(fui->leFMstatus->text() == "Waiting for trigger")
       {
           // If we received a trigger report the change status to running and
           // set the triggered flag
           // Looking for "DIC,x,RISING,time. where x is the input trigger
           // selected.
           res = comms->getline();
           if(res.startsWith("DIC,"))
           {
               reslist = res.split(",");
               if(reslist.count() == 4)
               {
                   if((reslist.at(2) == "RISING") && (reslist.at(1) == fui->comboFMtrig->currentText()))
                   {
                       CVparkingTriggered = true;
                       fui->leFMstatus->setText("Triggered");
                       eTimer.restart();
                   }
               }
           }
           return;
       }
       if(!CVparkingTriggered)
       {
           if( fui->tabMIPS->tabText(fui->tabMIPS->currentIndex()) == "FAIMS")
           {
              Update();
              Log("");
           }
           return;
       }
       CurrentMin = (float)eTimer.elapsed()/(float)60000;
       fui->leFMelasped->setText(QString::number(CurrentMin, 'f', 1));
       switch(State)
       {
          case WAITING:
           if(CurrentMin >= TargetRT - TargetWindow/2)
           {
               fui->leFMcompound->setText(TargetCompound);
               fui->leFMcvVolts->setText(QString::number(TargetCV));
               fui->leFMbiasVolts->setText(QString::number(TargetBias));
               comms->SendCommand("SFMCV," + QString::number(TargetCV) + "\n");
               comms->SendCommand("SFMBIAS," + QString::number(TargetBias) + "\n");
               State = SCANNING;
           }
           break;
          case SCANNING:
           if(CurrentMin >= TargetRT + TargetWindow/2)
           {
               if(!GetNextTarget(CurrentMin))
               {
                   CVparkingTriggered = false;
                   fui->pbFMstart->setText("Start");
                   fui->leFMstatus->setText("Finished");
               }
               if(CurrentMin < TargetRT - TargetWindow/2) fui->leFMcompound->setText("");
           }
           break;
          default:
            break;
       }
   }
   if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Linear scan")
   {
        if(WaitingForLinearScanTrig)
        {
            res = comms->getline();
            if(res.startsWith("DIC,"))
            {
                reslist = res.split(",");
                if(reslist.count() == 4)
                {
                    if((reslist.at(2) == "RISING") && (reslist.at(1) == fui->comboFMlinearTrig->currentText()))
                    {
                        WaitingForLinearScanTrig = false;
                        fui->statusBar->showMessage(tr("Scan triggered!"));
                        comms->SendCommand("SFMSTRTLIN,TRUE\n");
                        Log("Linear scan triggered!");
                    }
                }
            }
            return;
        }
   }
   if(fui->tabScanMode->tabText(fui->tabScanMode->currentIndex()) == "Step scan")
   {
        if(WaitingForStepScanTrig)
        {
            res = comms->getline();
            if(res.startsWith("DIC,"))
            {
                reslist = res.split(",");
                if(reslist.count() == 4)
                {
                    if((reslist.at(2) == "RISING") && (reslist.at(1) == fui->comboFMstepTrig->currentText()))
                    {
                        WaitingForStepScanTrig = false;
                        fui->statusBar->showMessage(tr("Scan triggered!"));
                        comms->SendCommand("SFMSTRTSTP,TRUE\n");
                        Log("Step scan triggered!");
                    }
                }
            }
            return;
        }
   }
   // If this FAIMS tab is displayed then update the parameters
   if( fui->tabMIPS->tabText(fui->tabMIPS->currentIndex()) == "FAIMS")
   {
      Update();
      Log("");
   }
}

void FAIMS::FAIMSstartLinearScan(void)
{
    QTime timer;

    if(comms == NULL) return;
    // If external trigger is requested then set flag indicating
    // we are waiting for a tigger and display message. If no external
    // trigger start as soon as the button is pressed.

    // Turn off all trigger reporting, just in case it was on
    comms->SendCommand("RPT,Q,OFF\n");
    comms->SendCommand("RPT,R,OFF\n");
    comms->SendCommand("RPT,S,OFF\n");
    comms->SendCommand("RPT,T,OFF\n");
    if(fui->comboFMlinearTrig->currentText() == "None")
    {
        if(fui->comboFMlinearTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,1\n");
        if(fui->comboFMlinearTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,1\n");
        if(fui->comboFMlinearTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,1\n");
        if(fui->comboFMlinearTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,0\n");
        if(fui->comboFMlinearTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,0\n");
        if(fui->comboFMlinearTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,0\n");
        fui->statusBar->showMessage(tr("Scan triggered!"));
        comms->SendCommand("SFMSTRTLIN,TRUE\n");
        Log("Linear scan started,CV start = " + fui->leSFMCVSTART->text() +
                               ",CV end = " + fui->leSFMCVEND->text() +
                               ",Duration, S = " + fui->leSFMDUR->text() +
                               ",Loops = " + fui->leSFMLOOPS->text());
        timer.start();
        while(timer.elapsed() < 250) QApplication::processEvents();
        if(fui->comboFMlinearTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,0\n");
        if(fui->comboFMlinearTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,0\n");
        if(fui->comboFMlinearTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,0\n");
        if(fui->comboFMlinearTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,1\n");
        if(fui->comboFMlinearTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,1\n");
        if(fui->comboFMlinearTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,1\n");
    }
    else
    {
        // Issue the report command to MIPS for the selected channel
        // and set status to waiting for trigger
        comms->SendCommand("RPT," + fui->comboFMlinearTrig->currentText() + ",RISING\n");
        fui->statusBar->showMessage(tr("Waiting for scan trigger!"));
        WaitingForLinearScanTrig = true;
        Log("Linear scan armed,CV start = " + fui->leSFMCVSTART->text() +
                             ",CV end = " + fui->leSFMCVEND->text() +
                             ",Duration, S = " + fui->leSFMDUR->text() +
                             ",Loops = " + fui->leSFMLOOPS->text());
    }
    fui->pbFMstartLinear->setEnabled(false);
    fui->pbFMabortLinear->setEnabled(true);
}

void FAIMS::FAIMSabortLinearScan(void)
{
    // Abort scan
    WaitingForLinearScanTrig = false;
    fui->statusBar->showMessage(tr("Scan aborted!"));
    comms->SendCommand("SFMSTRTLIN,FALSE\n");
    fui->pbFMstartLinear->setEnabled(true);
    fui->pbFMabortLinear->setEnabled(false);
    Log("Linear scan aborted!");
}

void FAIMS::FAIMSstartStepScan(void)
{
    QTime timer;

    if(comms == NULL) return;
    // If external trigger is requested then set flag indicating
    // we are waiting for a tigger and display message. If no external
    // trigger start as soon as the button is pressed.

    // Turn off all trigger reporting, just in case it was on
    comms->SendCommand("RPT,Q,OFF\n");
    comms->SendCommand("RPT,R,OFF\n");
    comms->SendCommand("RPT,S,OFF\n");
    comms->SendCommand("RPT,T,OFF\n");
    if(fui->comboFMstepTrig->currentText() == "None")
    {
        if(fui->comboFMstepTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,1\n");
        if(fui->comboFMstepTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,1\n");
        if(fui->comboFMstepTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,1\n");
        if(fui->comboFMstepTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,0\n");
        if(fui->comboFMstepTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,0\n");
        if(fui->comboFMstepTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,0\n");
        fui->statusBar->showMessage(tr("Scan triggered!"));
        comms->SendCommand("SFMSTRTSTP,TRUE\n");
        Log("Step scan started,CV start = " + fui->leSFMCVSTART_->text() +
                             ",CV end = " + fui->leSFMCVEND_->text() +
                             ",Step time mS = " + fui->leSFMSTPTM->text() +
                             ",Steps = " + fui->leSFMSTEPS->text() +
                             ",Loops = " + fui->leSFMLOOPS_->text());
        timer.start();
        while(timer.elapsed() < 250) QApplication::processEvents();
        if(fui->comboFMstepTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,0\n");
        if(fui->comboFMstepTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,0\n");
        if(fui->comboFMstepTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,0\n");
        if(fui->comboFMstepTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,1\n");
        if(fui->comboFMstepTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,1\n");
        if(fui->comboFMstepTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,1\n");
    }
    else
    {
        // Issue the report command to MIPS for the selected channel
        // and set status to waiting for trigger
        comms->SendCommand("RPT," + fui->comboFMstepTrig->currentText() + ",RISING\n");
        fui->statusBar->showMessage(tr("Waiting for scan trigger!"));
        WaitingForStepScanTrig = true;
        Log("Step scan armed,CV start = " + fui->leSFMCVSTART_->text() +
                           ",CV end = " + fui->leSFMCVEND_->text() +
                           ",Step time mS = " + fui->leSFMSTPTM->text() +
                           ",Steps = " + fui->leSFMSTEPS->text() +
                           ",Loops = " + fui->leSFMLOOPS_->text());
    }
    fui->pbFMstartLinear->setEnabled(false);
    fui->pbFMabortLinear->setEnabled(true);
}

void FAIMS::FAIMSabortStepScan(void)
{
    // Abort scan
    WaitingForStepScanTrig = false;
    fui->statusBar->showMessage(tr("Scan aborted!"));
    comms->SendCommand("SFMSTRTSTP,FALSE\n");
    fui->pbFMstartStep->setEnabled(true);
    fui->pbFMabortStep->setEnabled(false);
    Log("Step scan aborted!");
}

void FAIMS::FAIMSlockOff(void)
{
   comms->SendCommand("SFMLOCK,FALSE\n");
   Log("Output lock mode disabled requested.");
}

void FAIMS::FAIMSlockOn(void)
{
   comms->SendCommand("SFMLOCK,TRUE\n");
   Log("Output lock mode enable requested.");
}

void FAIMS::Log(QString Message)
{
    if(LogFileName == "") return;

    QFile file(LogFileName);
    if(file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&file);
        if(Message == "")
        {
            // Log the base set of opeating parameters
            QString LogString;
            LogString  = QDateTime::currentDateTime().toString() + ",";
            if(fui->chkFMenable->isChecked()) LogString += "Enabled,";
            else LogString += "Disabled,";
            LogString += fui->leSFMDRV->text() + ",";
            if(fui->rbSFMLOCK_FALSE->isChecked()) LogString += "Manual,";
            if(fui->rbSFMLOCK_TRUE->isChecked()) LogString += "Lock,";
            LogString += fui->leSFMSP->text() + ",";
            LogString += fui->leGFMPWR->text() + ",";
            LogString += fui->leGFMPV->text() + ",";
            LogString += fui->leGFMNV->text() + ",";
            LogString += fui->leGFMBIASA->text() + ",";
            LogString += fui->leGFMCVA->text() + ",";
            LogString += fui->leGFMOFFA->text() + "\n";
            stream << LogString;
        }
        else
        {
           stream << QDateTime::currentDateTime().toString() + ",";
           stream << Message + "\n";
        }
        file.close();
    }
}

void FAIMS::FAIMSselectLogFile(void)
{
    LogFileName = QFileDialog::getSaveFileName(this, tr("Log File"),"",tr("Settings (*.log);;All files (*.*)"));
    fui->LogFile->setText(LogFileName);
    QFile file(LogFileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# FAIMS log file, " + dateTime.toString() + "\n";
        stream << "Time,Enable,Drive,Mode,Voltage SP,Power,Pos KV,Neg KV,DC bias,DC CV,DC offset\n";
        file.close();
    }
}

void FAIMS::slotLinearTrigOut(void)
{
    if(comms == NULL) return;
    if(fui->comboFMlinearTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,0\n");
    if(fui->comboFMlinearTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,0\n");
    if(fui->comboFMlinearTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,0\n");
    if(fui->comboFMlinearTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,1\n");
    if(fui->comboFMlinearTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,1\n");
    if(fui->comboFMlinearTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,1\n");
}

void FAIMS::slotStepTrigOut(void)
{
    if(comms == NULL) return;
    if(fui->comboFMstepTrigOut->currentText() == "B") comms->SendCommand("SDIO,B,0\n");
    if(fui->comboFMstepTrigOut->currentText() == "C") comms->SendCommand("SDIO,C,0\n");
    if(fui->comboFMstepTrigOut->currentText() == "D") comms->SendCommand("SDIO,D,0\n");
    if(fui->comboFMstepTrigOut->currentText() == "B Active low") comms->SendCommand("SDIO,B,1\n");
    if(fui->comboFMstepTrigOut->currentText() == "C Active low") comms->SendCommand("SDIO,C,1\n");
    if(fui->comboFMstepTrigOut->currentText() == "D Active low") comms->SendCommand("SDIO,D,1\n");
}

void FAIMS::slotFAIMSautoTune(void)
{
    if(comms == NULL) return;
    comms->SendCommand("SFMTUNE\n");
}

void FAIMS::slotFAIMSautoTuneAbort(void)
{
    if(comms == NULL) return;
    comms->SendCommand("SFMTABRT\n");
}

void FAIMS::slotFAIMSCurtianEna(void)
{
    if(fui->chkCurtianEna->isChecked()) comms->SendCommand("SHVENA,1,TRUE\n");
    else  comms->SendCommand("SHVENA,1,FALSE\n");
}

void FAIMS::slotFAIMSCurtianInd(void)
{
    if(fui->chkCurtianIndEna->isChecked()) comms->SendCommand("SFMCCUR,FALSE\n");
    else  comms->SendCommand("SFMCCUR,TRUE\n");
}

void FAIMS::slotFAIMSnegTune(void)
{
    if(fui->chkNegTune->isChecked()) comms->SendCommand("SFMTPOS,FALSE\n");
    else  comms->SendCommand("SFMTPOS,TRUE\n");
}

