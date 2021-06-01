// Changes made to the timing generator to support external trigger mode.
// October 8, 2019
// 1.) Support time in mS by updating tonvert to counts function. Done
// 2.) When Enable trigger is pressed, download table and send messages waiting
//     for triggers. Do not use once mode. Done
// 3.) Use abort to exit table mode. No changes needed
// 4.) Look at file logic. Done
// 5.) Issue the command to allow MIPS to update when there is time. Use
//     the control startup init file to send this command. no changes needed.
// 6.) Add a clock freq box for time calculations when using mS mode. Done
// 7.) If in external trigger mode and trigger is pressed then ask if user would
//     like to force a trigger. Done
// 8.) AcquireData class updates:
//      - isRunning depends on the console window being present, needs to
//        use Acquiring flag. Done
// 9.) Update the save and load functions for methode file. Done
//      - Ext clock frequency
//      - Time mode check box

#include "timinggenerator.h"
#include "ui_timinggenerator.h"

// This function tests if the system is in table mode by sending the trigger source
// option. If its in table mode then a NAK will be received from MIPS
bool isTblMode(Comms *comms, QString TriggerSource)
{
   if(comms == NULL) return false;
   QString res = TriggerSource.toUpper();
   if(res == "SOFTWARE") res = "SW";
   comms->rb.clear();
   comms->SendString("STBLTRG," + res + "\n");
   comms->waitforline(1000);
   if(comms->rb.numLines() >= 1)
   {
       res = comms->rb.getline();
       if(res == "?") return true;
       if(res == "") return false;
   }
   return false;
}

// Download table parameters to MIPS.
// If the system is in table mode it will be placed in local mode.
// The table is downloaded and the system will remain in local mode.
// Returns false if an error is detected
bool DownloadTable(Comms *comms, QString Table, QString ClockSource, QString TriggerSource)
{
   if(comms == NULL) return false;
   comms->SendCommand("SMOD,LOC\n");        // Make sure the system is in local mode
   comms->SendCommand("STBLREPLY,FALSE\n"); // Turn off any table messages from MIPS
   // Make sure a table has been generated
   if(Table == "")
   {
       QMessageBox msgBox;
       msgBox.setText("There is no Pulse Sequence to download to MIPS!");
       msgBox.exec();
       return false;
   }
   // Set clock
   comms->SendCommand("STBLCLK," + ClockSource.toUpper() + "\n");
   // Set trigger
   QString res = TriggerSource.toUpper();
   if(res == "SOFTWARE") res = "SW";
   comms->SendCommand("STBLTRG," + res + "\n");
   // Send table
   comms->SendCommand(Table.trimmed());
   return true;
}

QString MakePathUnique(QString path)
{
    int i, num;

    // Check if path exists, if it does then see if it ends in a number, if
    // it does then increment this number, if it does not append -0001 to the
    // name
    while(true)
    {
       if(QDir(path).exists())
       {
           for(i=0;i<path.length();i++) if(!path[path.length()-1-i].isDigit()) break;
           if(i == 0) path += "-0001";
           else
           {
               num = path.right(i).toInt();
               num++;
               path = path.left(path.length()-i) += QString("%1").arg(num,4,10,QLatin1Char('0'));
           }
       }
       else break;
    }
    // At this point path is unique
    return path;
}

// **********************************************************************************************
// AcquireData implementation *******************************************************************
// **********************************************************************************************
AcquireData::AcquireData(QWidget *parent)
{
    p = parent;
    comms  = NULL;
    cla = NULL;
    Acquire = "";
    statusBar = NULL;
    properties = NULL;
    Acquiring = false;
    fileName = "U1084A.data";
    TriggerMode = "Software";
    LastFrameSize = 0;
    LastAccumulations = 0;
}

void AcquireData::StartAcquire(QString path, int FrameSize, int Accumulations)
{
    QString StatusMessage;

    StatusMessage.clear();
    // If the Acquire string is defined then we call the program defined
    // by The Acquire string. The program is called with optional arguments
    // as follows:
    // - Filename, this will result in a dialog box to appear
    //             allowing the user to select a filename to hold
    //             the data
    // - TOFscans, passes the total number of tof scans to acquire, this is
    //             the product of Frame size and accumulations
    // The acquire ap is expected to return "Ready" when ready to accept a trigger.
    if(properties != NULL) properties->Log("StartAcquire:" + path);
    filePath = "";
    // Make sure the system is in table mode
    QString TbkCmd = "SMOD,ONCE\n";
    if(TriggerMode != "Software") TbkCmd = "SMOD,TBL\n";
    if(comms == NULL) StatusMessage += "No open comms channels!\n";
    else if(comms->SendCommand(TbkCmd))
    {
        if(!TableDownloaded)
        {
            QString msg = "Pulse sequence table has not been downloaded to MIPS!";
            QMessageBox msgBox;
            msgBox.setText(msg);
            msgBox.setInformativeText("");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return;
        }
        if(statusBar != NULL) statusBar->showMessage("System mode changed to Table.", 5000);
        StatusMessage += "System mode changed to Table.\n";
    }
    else StatusMessage += "MIPS failed to enter table mode!\n";
    // Acquire contains the name and full path to the acquire command
    // line application. This is set in the controlpanel startup
    // function.
    if(Acquire != "")
    {
        QString cmd = "";
        QStringList resList = Acquire.split(",");
        cmd = Acquire;
        for(int i=0;i<resList.count();i++)
        {
            if(i==0) cmd = resList[0];
            if(resList[i].toUpper() == "ACCUMULATIONS")
            {
                cmd += " -A" + QString::number(Accumulations);
            }
            if(resList[i].toUpper() == "TOFSCANS")
            {
                cmd += " -S" + QString::number(FrameSize * Accumulations);
            }
            if(resList[i].toUpper() == "FILENAME")
            {
                // If the path is undefined popup and folder selection dialog
                while(path == "")
                {
                    CDirSelectionDlg *cds = new CDirSelectionDlg(QDir::currentPath(),p);
                    cds->setTitle("Select/enter folder to save data files");
                    cds->show();
                    while(cds->isVisible()) QApplication::processEvents();
                    if(cds->result() != 0)
                    {
                        QString selectedPath = cds->selectedPath();
                        delete cds;
                        // See if the directory is present
                        if(QDir(selectedPath).exists())
                        {
                            QString msg = "Selected folder exists, please define a unique folder to save data files.";
                            QMessageBox msgBox;
                            msgBox.setText(msg);
                            msgBox.setInformativeText("");
                            msgBox.setStandardButtons(QMessageBox::Ok);
                            msgBox.exec();
                        }
                        else
                        {
                            filePath = selectedPath;
                            break;
                        }
                    }
                    else
                    {
                       if(comms != NULL) comms->SendCommand("SMOD,LOC\n"); // Return to local mode
                       return;
                    }
                }
                // Create the folder and define the data storage path and file
                if(path != "")
                {
                    if(properties->AutoFileName) filePath = MakePathUnique(path);
                    else
                    {
                        filePath = path;
                        if(QDir(filePath).exists())
                        {
                            if(statusBar != NULL) statusBar->showMessage("File path not unique, please try again!", 5000);
                            if(comms != NULL) comms->SendCommand("SMOD,LOC\n"); // Return to local mode
                            return;
                        }
                    }
                }
                if(!QDir().mkdir(filePath))
                {
                    // If here the defined path cannot be created so warn the user!
                    QMessageBox msgBox;
                    QString msg = "The path you have defined cannot be created. You will need to ";
                    msg += "abort this acquisition and restart with a valid path. The path you defined ";
                    msg += "is: " + filePath + "\n";
                    msgBox.setText(msg);
                    msgBox.exec();
                }
                QDir().setCurrent(filePath);
                cmd += " " + filePath + "/" + fileName;
            }
        }
        if(cla != NULL)
        {
            if((LastFrameSize != FrameSize) || (LastAccumulations != Accumulations))
            {
                disconnect(cla,SIGNAL(Ready()),0,0);
                disconnect(cla,SIGNAL(AppCompleted()),0,0);
                disconnect(cla,SIGNAL(DialogClosed()),0,0);
                delete cla;
                cla = NULL;
            }
        }
        if(cla == NULL)
        {
           // Here is the command line window has not been created or destroved by
           // the user.
           cla = new cmdlineapp(p);
           // connect(cla,SIGNAL(Ready()),this,SLOT(slotAppReady()),Qt::QueuedConnection);
           // connect(cla,SIGNAL(AppCompleted()),this,SLOT(slotAppFinished()),Qt::QueuedConnection);
           // connect(cla,SIGNAL(DialogClosed()),this,SLOT(slotDialogClosed()),Qt::QueuedConnection);
           connect(cla,SIGNAL(Ready()),this,SLOT(slotAppReady()));
           connect(cla,SIGNAL(AppCompleted()),this,SLOT(slotAppFinished()));
           connect(cla,SIGNAL(DialogClosed()),this,SLOT(slotDialogClosed()));
        }
        cla->appPath = cmd;
        fileName.remove(" -L");
        fileName.remove(" -l");
        cla->appPathNoOptions = filePath + "/" + fileName;
        emit dataFileDefined(cla->appPathNoOptions);
        cla->Clear();
        cla->show();
        cla->raise();
        cla->AppendText(StatusMessage);
        cla->ReadyMessage = "Ready";
        cla->InputRequest = "? Y/N :";
        cla->ContinueMessage = "Acquire complete, continue?: ";
        if(filePath != "") cla->fileName = filePath + "/Acquire.data";
        cla->Execute();
        Acquiring = true;
        if(properties != NULL) properties->Log("Aquire app started: " + filePath + "/Acquire.data");
        LastFrameSize = FrameSize;
        LastAccumulations = Accumulations;
    }
    else
    {
        if(comms == NULL) return;
        // Send table start command if in software trigger mode else we just wait
        // for an external trigger
        if(TriggerMode == "Software")
        {
           if(comms->SendCommand("TBLSTRT\n")) {if(statusBar != NULL) statusBar->showMessage("Table trigger command accepted!", 5000);}
           else if(statusBar != NULL) statusBar->showMessage("Table trigger command rejected!", 5000);
        }
    }
}

void AcquireData::slotDialogClosed(void)
{
// These disconnects cause it to crash if connect is Qt::QueuedConnection, ??
//    disconnect(cla,SIGNAL(Ready()),0,0);
//    disconnect(cla,SIGNAL(AppCompleted()),0,0);
//    disconnect(cla,SIGNAL(DialogClosed()),0,0);
    if(properties != NULL) properties->Log("Aquire application dialog was closed!");
    cla = NULL;
}

bool AcquireData::isRunning(void)
{
   // Oct 8, 2019. Changed this to use the MIPS system table state
   if(cla != NULL) cla->raise();
   if(isTblMode(comms, TriggerMode)) return(true);
   return(false);
   // Old logic
   if(cla == NULL) return(false);
   if(cla->process.state() == QProcess::NotRunning) return(false);
   cla->raise();
   return(true);
}

void AcquireData::slotAppReady(void)
{
    if(properties != NULL) properties->Log("Aquire app ready");
    if(comms == NULL) return;
    // Send table start command
    if(comms->SendCommand("TBLSTRT\n"))
    {
        if(statusBar != NULL) statusBar->showMessage("Table trigger command accepted!", 5000);
        if(properties != NULL) properties->Log("Table triggered");
    }
    else
    {
        if(statusBar != NULL) statusBar->showMessage("Table trigger command rejected!", 5000);
        if(properties != NULL) properties->Log("Table trigger failed");
    }
}

void AcquireData::slotAppFinished(void)
{
    if(properties != NULL) properties->Log("Aquire finished");
    // Send a signal that the data collection has finished.
    Acquiring = false;
    if(comms == NULL) return;
    if(TriggerMode == "Software")
    {
        comms->SendCommand("SMOD,LOC\n");
        if(statusBar != NULL) statusBar->showMessage("Acquire app finished, returning to local mode.", 5000);
    }
    else
    {
        if(statusBar != NULL) statusBar->showMessage("Acquire app finished, waiting for next trigger", 5000);
        // This needs work for sure!
    }
    emit dataAcquired(filePath);
}

void AcquireData::Dismiss(void)
{
    if(cla == NULL) return;
    cla->Dismiss();
}


// *************************************************************************************************
// TimingControl implementation  *******************************************************************
// *************************************************************************************************

TimingControl::TimingControl(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    Acquire = "";
    statusBar = NULL;
    properties = NULL;
    Acquiring = false;
    Downloading = false;
    fileName = "U1084A.data";
    FrameCtAdj = 1;
    AlwaysGenerate = false;
}

void TimingControl::Show(void)
{
    // Make a group box
    gbTC = new QGroupBox(Title,p);
    gbTC->setGeometry(X,Y,140,120);
    gbTC->setToolTip(MIPSnm + " Timing generation");
    // Place the controls on the group box
    Edit = new QPushButton("Edit",gbTC);       Edit->setGeometry(20,25,100,32); Edit->setAutoDefault(false);
    Trigger = new QPushButton("Trigger",gbTC); Trigger->setGeometry(20,55,100,32); Trigger->setAutoDefault(false);
    Abort = new QPushButton("Abort",gbTC);     Abort->setGeometry(20,85,100,32); Abort->setAutoDefault(false);
    // Connect to the event slots
    connect(Edit,SIGNAL(pressed()),this,SLOT(pbEdit()),Qt::QueuedConnection);
    connect(Trigger,SIGNAL(pressed()),this,SLOT(pbTrigger()),Qt::QueuedConnection);
    connect(Abort,SIGNAL(pressed()),this,SLOT(pbAbort()),Qt::QueuedConnection);
    TableDownloaded = false;
    TG = new TimingGenerator(p,Title,MIPSnm);
    TG->comms = comms;
    TG->Trigger = Trigger;
    TG->Abort = Abort;
    TG->Edit = Edit;
    TG->properties = properties;
    TG->FrameCtAdj = FrameCtAdj;
    // Create the AcquireData object and init
    AD = new class AcquireData(p);
    AD->fileName = fileName;
    AD->comms = comms;
    AD->statusBar = statusBar;
    AD->properties = properties;
    connect(AD,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
    connect(AD,SIGNAL(dataFileDefined(QString)),this,SLOT(slotDataFileDefined(QString)));
}

void TimingControl::pbEdit(void)
{
   Edit->setDown(false);
   TG->show();
   TG->raise();
}

void TimingControl::pbTrigger(void)
{
    Trigger->setDown(false);
    if(AD->TriggerMode == "Software")
    {
        if(AD->isRunning())
        {
            if(properties != NULL) properties->Log("Timing control trigger pressed while running!");
            return;
        }
        if(properties != NULL) properties->Log("Timing control trigger pressed.");
        //if(TG->isTableMode())
        //{
        //    if(statusBar != NULL) statusBar->showMessage("Can't trigger, system in table mode!", 5000);
        //    return;
        //}
    }
    else
    {
        // Here if in external trigger mode so test if we are in table mode, if
        // so ask if user would like to force a trigger.
        if(AD->isRunning())
        {
            if(properties != NULL) properties->Log("Timing control trigger pressed while running!");
            // Now ask user about forcing a trigger
            QMessageBox msgBox;
            QString msg = "The system is in external trigger mode waiting for a trigger event.\n";
            msg += "Select Yes if you would like to force a trigger or No to exit and keep waiting.\n";
            msgBox.setText(msg);
            msgBox.setInformativeText("Select Yes to force a trigger?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if(ret == QMessageBox::Yes)
            {
                if(properties != NULL) properties->Log("Timing control trigger forced.");
                if(comms->SendCommand("TBLSTRT\n")) {if(statusBar != NULL) statusBar->showMessage("Table trigger command accepted!", 5000);}
                else if(statusBar != NULL) statusBar->showMessage("Table trigger command rejected!", 5000);
                return;
            }
            else return;
        }
        if(properties != NULL) properties->Log("Timing control trigger pressed, external triggers enabled.");
    }
    // If the table line edit box is empty, generate table
    AD->fileName = fileName;
    AD->TriggerMode = TG->ui->comboTriggerSource->currentText();
    if(properties != NULL) properties->Log("Trigger mode: " + AD->TriggerMode);
    if(AlwaysGenerate) TG->ui->leTable->setText("");
    if(TG->ui->leTable->text() == "") TG->slotGenerate();
    Downloading = true;
    // Set the external clock frequency
    if(comms != NULL) comms->SendCommand("SEXTFREQ," + TG->ui->leExtClkFreq->text() + "\n");
    // Send table
    TableDownloaded = DownloadTable(comms, TG->ui->leTable->text(),TG->ui->comboClockSource->currentText(), TG->ui->comboTriggerSource->currentText());
    TG->UpdateEvents();
    Downloading = false;
    if(properties != NULL)
    {
        if((properties->DataFilePath != "") && (properties->FileName != "")) AcquireData(properties->DataFilePath + "/" + properties->FileName);
        else AcquireData("");
    }
    else AcquireData("");
}

void TimingControl::slotDataAcquired(QString filepath)
{
    emit dataAcquired(filepath);
}

void TimingControl::slotDataFileDefined(QString filepath)
{
    emit dataFileDefined(filepath);
}

void TimingControl::slotEventChanged(QString ECname, QString Val)
{
    QStringList resList;

    //qDebug() << ECname << "," << Val;
    resList = ECname.split(".");
    if(resList.count() != 2) return;
    foreach(Event *e, TG->Events)
    {
        if(e->Name.toUpper() == resList[0].toUpper())
        {
            if(resList[1].toUpper() == "VALUE")
            {
                e->Value = Val.toFloat();
                if(TG->ui->leEventName->text() == resList[0]) TG->ui->leEventValue->setText(Val);
            }
            if(resList[1].toUpper() == "VALUEOFF")
            {
                e->ValueOff = Val.toFloat();
                if(TG->ui->leEventName->text() == resList[0]) TG->ui->leEventValueOff->setText(Val);
            }
        }
    }

}

void TimingControl::AcquireData(QString path)
{
    AD->TableDownloaded = TableDownloaded;
    AD->StartAcquire(path, TG->ConvertToCount(TG->ui->leFrameWidth->text()), TG->ui->leAccumulations->text().toInt());
}

void TimingControl::pbAbort(void)
{
    Abort->setDown(false);
    if(comms == NULL) return;
    // Send table abort command
    if(comms->SendCommand("TBLABRT\n")) { if(statusBar != NULL) statusBar->showMessage("Table mode aborted!", 5000); }
    else if(statusBar != NULL) statusBar->showMessage("Table mode abort error!", 5000);
}

// *************************************************************************************************
// EventControl implementation     *****************************************************************
// *************************************************************************************************
EventControl::EventControl(QWidget *parent, QString name, QString Ename, int x, int y)
{
    p      = parent;
    Title  = name;
    ECname = Ename;
    X      = x;
    Y      = y;
    comms  = NULL;
    Gcommand.clear();
    Scommand.clear();
}

void EventControl::Show(void)
{
    frmEvent = new QFrame(p); frmEvent->setGeometry(X,Y,170,21);
    EventValue = new QLineEdit(frmEvent); EventValue->setGeometry(70,0,70,21); EventValue->setValidator(new QDoubleValidator);
    label = new QLabel(Title,frmEvent); label->setGeometry(0,0,59,16);
    EventValue->setToolTip("Timimg generator event value editor, " + ECname);
    connect(EventValue,SIGNAL(editingFinished()),this,SLOT(EventChange()));
}

void EventControl::Update(void)
{
    QString res;

    if(Gcommand.isEmpty()) return;
    if(comms == NULL) return;
    comms->rb.clear();
    res = Gcommand + "\n";
    res = comms->SendMess(res);
    if(res == "") return;
    if(!EventValue ->hasFocus()) EventValue->setText(res);
}

QString EventControl::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + "," + EventValue->text();
    return(res);
}

bool EventControl::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    EventValue->setText(resList[1]);
    EventValue->setModified(true);
    EventValue->editingFinished();
    return true;
}

// Sets or returns this controls value
QString EventControl::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title) return EventValue->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       EventValue->setText(resList[1]);
       EventValue->setModified(true);
       EventValue->editingFinished();
       return "";
    }
    return "?";
}

void EventControl::EventChange(void)
{
    emit EventChanged(ECname,EventValue->text());
    if(Scommand.isEmpty()) return;
    //if(comms == NULL) return;
    if(!EventValue->isModified()) return;
    QString res = Scommand + "," + EventValue->text() + "\n";
//    qDebug() << res;
    if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
    EventValue->setModified(false);
}

// *************************************************************************************************
// TimingGenerator implementation  *****************************************************************
// *************************************************************************************************

TimingGenerator::TimingGenerator(QWidget *parent, QString name, QString MIPSname) :
    QDialog(parent),
    ui(new Ui::TimingGenerator)
{
    ui->setupUi(this);

    p         = parent;
    Title     = name;
    MIPSnm    = MIPSname;
    comms     = NULL;
    statusBar = NULL;
    Events.clear();
    EC.clear();
    selectedEvent = NULL;

    QWidget::setWindowTitle(Title + " editor");
    ui->comboClockSource->clear();
    ui->comboClockSource->addItem("Ext","Ext");
    ui->comboClockSource->addItem("ExtN","ExtN");
    ui->comboClockSource->addItem("ExtS","ExtS");
    ui->comboClockSource->addItem("42000000","42000000");
    ui->comboClockSource->addItem("10500000","10500000");
    ui->comboClockSource->addItem("2625000","2625000");
    ui->comboClockSource->addItem("656250","656250");
    ui->comboTriggerSource->clear();
    ui->comboTriggerSource->addItem("Software","Software");
    ui->comboTriggerSource->addItem("Edge","Edge");
    ui->comboTriggerSource->addItem("Pos","Pos");
    ui->comboTriggerSource->addItem("Neg","Neg");
    ui->comboEnable->clear();
    ui->comboEnable->addItem("","");
    ui->comboEnable->addItem("A","A");
    ui->comboEnable->addItem("B","B");
    ui->comboEnable->addItem("C","C");
    ui->comboEnable->addItem("D","D");
    ui->comboSelectEvent->clear();
    ui->comboSelectEvent->addItem("");
    ui->comboSelectEvent->addItem("New event");
    ui->comboSelectEvent->addItem("Rename event");
    ui->comboSelectEvent->addItem("Delete current");
    ui->comboEventSignal->clear();
    ui->comboEventSignal->addItem("","");

    connect(ui->comboSelectEvent,SIGNAL(currentIndexChanged(int)),this,SLOT(slotEventChange()));
    connect(ui->leEventStart,SIGNAL(editingFinished()),this,SLOT(slotEventUpdated()));
    connect(ui->leEventWidth,SIGNAL(editingFinished()),this,SLOT(slotEventUpdated()));
    connect(ui->leEventValue,SIGNAL(editingFinished()),this,SLOT(slotEventUpdated()));
    connect(ui->leEventValueOff,SIGNAL(editingFinished()),this,SLOT(slotEventUpdated()));
    connect(ui->comboEventSignal,SIGNAL(currentIndexChanged(int)),this,SLOT(slotEventUpdated()));
    connect(ui->pbGenerate,SIGNAL(pressed()),this,SLOT(slotGenerate()));
    connect(ui->pbClearEvents,SIGNAL(pressed()),this,SLOT(slotClearEvents()));
    connect(ui->pbLoad,SIGNAL(pressed()),this,SLOT(slotLoad()));
    connect(ui->pbSave,SIGNAL(pressed()),this,SLOT(slotSave()));

    // The following events will signal a new table needs to be generated so the slot
    // will clear the table field
    connect(ui->comboMuxOrder,SIGNAL(currentIndexChanged(int)),this,SLOT(slotClearTable()));
    connect(ui->comboClockSource,SIGNAL(currentIndexChanged(int)),this,SLOT(slotClearTable()));
    connect(ui->comboTriggerSource,SIGNAL(currentIndexChanged(int)),this,SLOT(slotClearTable()));
    connect(ui->chkTimeMode,SIGNAL(clicked(bool)),this,SLOT(slotClearTable()));
}

TimingGenerator::~TimingGenerator()
{
    //qDebug() << "Kill TG";
    delete ui;
}

// Called after table is downloaded to MIPS. This function will
// update all the event control parapeters and connect to the
// pulse sequence events.
void TimingGenerator::UpdateEvents(void)
{
    QStringList resList;

    foreach(EventControl *ec, EC)
    {
        resList = ec->ECname.split(".");
        ec->comms = comms;
        if(resList.count() == 2) foreach(Event *evt, Events)
        {
            if(resList[0].toUpper()==evt->Name.toUpper())
            {
                if(resList[1].toUpper()=="VALUE")
                {
                    ec->Scommand = "STBLVLT,"+QString::number((int)evt->StartT)+","+evt->Channel;
                    ec->Gcommand = "GTBLVLT,"+QString::number((int)evt->StartT)+","+evt->Channel;
                    ec->EventValue->setText(QString::number(evt->Value));
                }
                else if(resList[1].toUpper()=="VALUEOFF")
                {
                    ec->Scommand = "STBLVLT,"+QString::number((int)(evt->StartT+evt->WidthT))+","+evt->Channel;
                    ec->Gcommand = "GTBLVLT,"+QString::number((int)(evt->StartT+evt->WidthT))+","+evt->Channel;
                    ec->EventValue->setText(QString::number(evt->ValueOff));
                }
                else
                {
                    ec->Scommand.clear();
                    ec->Gcommand.clear();
                }
            }
        }
        else
        {
            ec->Scommand.clear();
            ec->Gcommand.clear();
        }
    }
}

// Reports all the setting to be saved in methodes file or sequence file
QString TimingGenerator::Report(void)
{
    QString res;

    res.clear();
    // Report all of the events;
    foreach(Event *evt, Events)
    {
        res += "TGevent," + Title + "," + evt->Name + "," + evt->Channel + ",";
        res += evt->Start + "," + evt->Width + ",";
        res += QString::number(evt->Value) + "," + QString::number(evt->ValueOff) + "\n";
    }
    // Frame parameters
    res += "TGframe," + Title + "," + ui->leFrameStart->text() + "," + ui->leFrameWidth->text() + ",";
    res += ui->leAccumulations->text() + "," + ui->comboEnable->currentText() + "\n";
    // Clock and trigger settings
    res += "TGclock," + Title + "," + ui->comboClockSource->currentText() + "\n";
    res += "TGtrigger," + Title + "," + ui->comboTriggerSource->currentText() + "\n";
    // Table
    res += "TGmux," + Title + "," + ui->comboMuxOrder->currentText() + "\n";
    res += "TGtable," + Title + "," + ui->leTable->text() + "\n";
    // External clock
    res += "ExtClock," + Title + "," + ui->leExtClkFreq->text() + "\n";
    // Time mode
    if(ui->chkTimeMode->isChecked()) res += "TimeMode," + Title + ",TRUE\n";
    else res += "TimeMode," + Title + ",FALSE\n";
//    res += "TCparametersEnd\n";
    return res;
}

bool TimingGenerator::SetValues(QString strVals)
{
    QStringList resList;

    if(strVals.startsWith("TGevent," + Title + ","))
    {
       resList = strVals.split(",");
       if(resList.count() != 8) return false;
       Event *event = new Event();
       event->Name = resList[2];
       event->Channel = resList[3];
       event->Start = resList[4];
       event->Width = resList[5];
       event->Value = resList[6].toFloat();
       event->ValueOff = resList[7].toFloat();
       Events.append(event);
       if(Events.count()==1)
       {
           disconnect(ui->comboSelectEvent, SIGNAL(currentIndexChanged(int)), 0, 0);
           ui->comboSelectEvent->clear();
           ui->comboSelectEvent->addItem("");
           ui->comboSelectEvent->addItem("New event");
           ui->comboSelectEvent->addItem("Rename event");
           ui->comboSelectEvent->addItem("Delete current");
           connect(ui->comboSelectEvent,SIGNAL(currentIndexChanged(int)),this,SLOT(slotEventChange()));
       }
       ui->comboSelectEvent->addItem(resList[2]);
//       connect(ui->comboSelectEvent,SIGNAL(currentIndexChanged(int)),this,SLOT(slotEventChange()));
       return true;
    }
    if(strVals.startsWith("TGframe," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 6) return false;
        ui->leFrameStart->setText(resList[2]);
        ui->leFrameWidth->setText(resList[3]);
        ui->leAccumulations->setText(resList[4]);
        ui->comboEnable->setCurrentIndex(ui->comboEnable->findText(resList[5]));
        return true;
    }
    if(strVals.startsWith("TGclock," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 3) return false;
        ui->comboClockSource->setCurrentIndex(ui->comboClockSource->findText(resList[2]));
        return true;
    }
    if(strVals.startsWith("TGtrigger," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 3) return false;
        ui->comboTriggerSource->setCurrentIndex(ui->comboTriggerSource->findText(resList[2]));
        return true;
    }
    if(strVals.startsWith("TGmux," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 3) return false;
        ui->comboMuxOrder->setCurrentIndex(ui->comboMuxOrder->findText(resList[2]));
        return true;
    }
    if(strVals.startsWith("TGtable," + Title + ","))
    {
        int i = strVals.indexOf("STBLDAT");
        if(i > 0) ui->leTable->setText(strVals.mid(i));
        return true;
    }
    if(strVals.startsWith("ExtClock," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 3) return false;
        ui->leExtClkFreq->setText(resList[2]);
        return true;
    }
    if(strVals.startsWith("TimeMode," + Title + ","))
    {
        resList = strVals.split(",");
        if(resList.count() != 3) return false;
        if(resList[2] == "TRUE") ui->chkTimeMode->setChecked(true);
        if(resList[2] == "FALSE") ui->chkTimeMode->setChecked(false);
        return true;
    }
    return false;
}

bool TimingGenerator::isTableMode(void)
{
    return isTblMode(comms,ui->comboTriggerSource->currentText());
}

void TimingGenerator::AddSignal(QString title, QString chan)
{
    ui->comboEventSignal->addItem(title,chan);
}

QStringList TimingGenerator::Split(QString str, QString del)
{
    QString     s;
    QStringList reslist;

    reslist.clear();
    s.clear();
    for(int i= 0;i<str.count();i++)
    {
        if(str.mid(i,1) == " ") continue;
        if(del.contains(str.mid(i,1)))
        {
            if(!s.isEmpty()) reslist.append(s);
            s.clear();
            reslist.append(str.mid(i,1));
        }
        else s += str.mid(i,1);
    }
    if(!s.isEmpty()) reslist.append(s);
    return(reslist);
}
int TimingGenerator::ConvertToCount(QString val)
{
    bool  ok;
    float clock  = 0;
    float result;

    result = CalculateTime(val);
    if(ui->chkTimeMode->isChecked())
    {
        clock = ui->comboClockSource->currentText().toInt(&ok);
        if(ok) result = result * (clock/1000.0);
        else
        {
            clock = ui->leExtClkFreq->text().toInt(&ok);
            if(ok) result = result * (clock/1000.0);
        }
    }
    return result;
}

float TimingGenerator::CalculateTime(QString val)
{
    QStringList reslist;
    bool  ok;
    float result = 0;
    float j      = 0;
    int   sign   = 1;

    reslist = Split(val,"+-");
    for(int i=0;i<reslist.count();i++)
    {
        j = reslist[i].toFloat(&ok);
        if(ok) result += sign * j;
        else if(reslist[i] == "+") sign =  1;
        else if(reslist[i] == "-") sign = -1;
        else
        {
            foreach(Event *evt, Events)
            {
                QString evtName = evt->Name;
                evtName.replace(" ", "");
                if(evtName == reslist[i])
                {
                    result += sign * CalculateTime(evt->Start);
                    break;
                }
                else if((evtName + ".Start") == reslist[i])
                {
                    result += sign * CalculateTime(evt->Start);
                    break;
                }
                else if((evtName + ".Width") == reslist[i])
                {
                    result += sign * CalculateTime(evt->Width);
                    break;
                }
            }
        }
    }
    return(result);
}

QString TimingGenerator::GenerateMuxSeq(QString Seq)
{
    int maxCount = ConvertToCount(ui->leFrameWidth->text()) + ConvertToCount(ui->leFrameStart->text());
    bool timeFlag;
    QString table;
    QList<int> InjectionPoints;

    if(properties != NULL) properties->Log("Mux seq: " + Seq);
    int l = Seq.length();
    if(l <= 0) return "";
    // Look for 1s in the sequence and build a list of
    // TOF counts for injections
    for(int i=0;i<l;i++) if(Seq[i] == '1') InjectionPoints.append((i * ConvertToCount(ui->leFrameWidth->text()) / (l))  + ConvertToCount(ui->leFrameStart->text()));
    slotEventUpdated();
    table.clear();
    if(ui->comboTriggerSource->currentText() == "Software") table = "STBLDAT;0:[A:" + QString::number(ui->leAccumulations->text().toInt() + FrameCtAdj);
    else table = "STBLDAT;0:[A:" + QString::number(ui->leAccumulations->text().toInt());
    foreach(Event *evt, Events)
    {
        if(evt->Channel.isEmpty()) continue;
        evt->StartT = ConvertToCount(evt->Start);
        evt->WidthT = ConvertToCount(evt->Width);
    }
    // Do some validation tests, make sure the required events are defined, also make sure the sum of
    // fill time, trap time, and release time will fit between the two closest injection points
    int events_found=0;
    int TotalInjectEventTime = 0;
    int InjectTime = 0;
    foreach(Event *evt, Events)
    {
        if(evt->Name == "Fill time") {events_found++; TotalInjectEventTime += ConvertToCount(evt->Width);}
        if(evt->Name == "Trap time") {events_found++; TotalInjectEventTime += ConvertToCount(evt->Width);}
        if(evt->Name == "Release time") {events_found++; TotalInjectEventTime += ConvertToCount(evt->Width);}
        if(evt->Name == "Inject time") {events_found++; InjectTime = ConvertToCount(evt->Start);}
    }
    if((events_found != 4) || (maxCount/l < TotalInjectEventTime) || (InjectTime != 0))
    {
        QMessageBox msgBox;
        QString msg = "Invalid pulse sequence! The following requirements\n";
        msg +=        "must be met:\n";
        msg +=        "  The following events must be defined:\n";
        msg +=        "       Fill Time, \n";
        msg +=        "       Trap time, \n";
        msg +=        "       Release time, \n";
        msg +=        "       Inject time, \n";
        msg +=        "  Inject time Start must equal 0.\n";
        msg +=        "  Fill time, Trap time, and Release time are relative\n";
        msg +=        "  to Inject time.\n";
        msg +=        "  The sum of Fill time, Trap time, and release time\n";
        msg +=        "  widths must be less than\n";
        msg +=        "  frame width / 2^(mux order -1).\n";
        msgBox.setText(msg);
        msgBox.exec();
        return "";
    }
    // End of validation testing
    int FrameStartT = ConvertToCount(ui->leFrameStart->text());
    for(int i=0;i <= maxCount;i++)
    {
        timeFlag = false;
        // Search for event at this clock cycle
        foreach(Event *evt, Events)
        {
            if(evt->Channel.isEmpty()) continue;
            int et = evt->StartT;
            bool Matched = false;
            if((evt->Name == "Fill time") || (evt->Name == "Trap time") || (evt->Name == "Release time"))
            {
                foreach(int it, InjectionPoints) if((et + it) == i) Matched = true;
            }
            else if(et == i) Matched = true;
            if(Matched)
            {
                if(!timeFlag) { table += "," + QString::number(i); timeFlag=true; }
                table += ":" + evt->Channel + ":" + QString::number(evt->Value);
            }
            et = evt->StartT + evt->WidthT;
            Matched = false;
            if((evt->Name == "Fill time") || (evt->Name == "Trap time") || (evt->Name == "Release time"))
            {
                foreach(int it, InjectionPoints) if((et + it) == i) Matched = true;
            }
            else if(et == i) Matched = true;
            if(Matched)
            {
                if(evt->WidthT > 0)
                {
                   if(!timeFlag) { table += "," + QString::number(i); timeFlag=true; }
                   table += ":" + evt->Channel + ":" + QString::number(evt->ValueOff);
                }
            }
        }
        if(FrameStartT == i)
        {
            if(ui->comboEnable->currentText() != "")
            {
                if(!timeFlag) { table += "," + QString::number(i); timeFlag=true; }
                table += ":" + ui->comboEnable->currentText() + ":1";
            }
        }
        if(maxCount == i)
        {
            if(!timeFlag) { table += "," + QString::number(i); timeFlag=true; }
            if(ui->comboEnable->currentText() != "")
            {
               table += ":" +  ui->comboEnable->currentText() + ":0";
            }
            table += ":];";
        }
    }
    return table;
}

void  TimingGenerator::slotGenerate(void)
{
    int maxCount = ConvertToCount(ui->leFrameWidth->text()) + ConvertToCount(ui->leFrameStart->text());
    bool timeFlag;
    QString table;

    if(ui->comboMuxOrder->currentText() == "4 Bit") ui->leTable->setText(GenerateMuxSeq("000100110101111"));
    else if(ui->comboMuxOrder->currentText() == "5 Bit") ui->leTable->setText(GenerateMuxSeq("0000100101100111110001101110101"));
    else if(ui->comboMuxOrder->currentText() == "6 Bit") ui->leTable->setText(GenerateMuxSeq("000001000011000101001111010001110010010110111011001101010111111"));
    else if(ui->comboMuxOrder->currentText() == "7 Bit") ui->leTable->setText(GenerateMuxSeq("0000001000001100001010001111001000101100111010100111110100001110001001001101101011011110110001101001011101110011001010101111111"));
    else if(ui->comboMuxOrder->currentText() == "8 Bit") ui->leTable->setText(GenerateMuxSeq("000000010111000111011110001011001101100001111001110000101011111111001011110100101000011011101101111101011101000001100101010100011010110001100000100101101101010011010011111101110011001111011001000010000001110010010011000100111010101101000100010100100011111"));
    else if(ui->comboMuxOrder->currentText() == "9 Bit") ui->leTable->setText(GenerateMuxSeq("0000000010000100011000010011100101010110000110111101001101110010001010000101011010011111101100100100101101111110010011010100110011000000011000110010100011010010111111101000101100011101011001011001111000111110111010000011010110110111011000001011010111110101010100000010100101011110010111011100000011100111010010011110101110101000100100001100111000010111101101100110100001110111100001111111110000011110111110001011100110010000010010100111011010001111001111100110110001010100100011100011011010101110001001100010001"));
    if(ui->comboMuxOrder->currentText() != "None") return;
    slotEventUpdated();
    table.clear();
    if(ui->comboTriggerSource->currentText() == "Software") table = "STBLDAT;0:[A:" + QString::number(ui->leAccumulations->text().toInt() + FrameCtAdj);
    else table = "STBLDAT;0:[A:" + QString::number(ui->leAccumulations->text().toInt());
    foreach(Event *evt, Events)
    {
        if(evt->Channel.isEmpty()) continue;
        evt->StartT = ConvertToCount(evt->Start);
        evt->WidthT = ConvertToCount(evt->Width);
    }
    int FrameStartT = ConvertToCount(ui->leFrameStart->text());
    for(int i=0;i <= abs(maxCount);i++)
    {
        timeFlag = false;
        // Search for event at this clock cycle
        foreach(Event *evt, Events)
        {
            if(evt->Channel.isEmpty()) continue;
            if((int)abs(evt->StartT) == i)
            {
                if(!timeFlag) { table += "," + QString::number((int)evt->StartT); timeFlag=true; }
                table += ":" + evt->Channel + ":" + QString::number(evt->Value);
            }
            if((int)abs(evt->StartT + evt->WidthT) == i)
            {
                if(abs(evt->WidthT) > 0)
                {
                   if(!timeFlag) { table += "," + QString::number((int)(evt->StartT + evt->WidthT)); timeFlag=true; }
                   table += ":" + evt->Channel + ":" + QString::number(evt->ValueOff);
                }
            }
        }
        if((int)abs(FrameStartT) == i)
        {
            if(ui->comboEnable->currentText() != "")
            {
                if(!timeFlag) { table += "," + QString::number((int)FrameStartT); timeFlag=true; }
                table += ":" + ui->comboEnable->currentText() + ":1";
            }
        }
        if(abs(maxCount) == i)
        {
            if(!timeFlag) { table += "," + QString::number((int)maxCount); timeFlag=true; }
            if(ui->comboEnable->currentText() != "")
            {
               table += ":" +  ui->comboEnable->currentText() + ":0";
            }
            table += ":];";
        }
    }
    ui->leTable->setText(table);
}

void TimingGenerator::slotEventUpdated(void)
{
    if(selectedEvent == NULL) return;
    selectedEvent->Name = ui->leEventName->text();
    selectedEvent->Channel = ui->comboEventSignal->currentData().toString();
    selectedEvent->Start = ui->leEventStart->text();
    selectedEvent->Width = ui->leEventWidth->text();
    selectedEvent->Value = ui->leEventValue->text().toFloat();
    selectedEvent->ValueOff = ui->leEventValueOff->text().toFloat();
}

void TimingGenerator::slotEventChange(void)
{
    bool ok;
    QString text;

    if(ui->comboSelectEvent->currentText() == "New event")
    {
        while(true)
        {
            text = QInputDialog::getText(0, "New event", "Enter event name, must be unique:", QLineEdit::Normal,"", &ok );
            if (ok && !text.isEmpty() )
            {
                if(ui->comboSelectEvent->findText(text) >= 0)
                {
                    QMessageBox msgBox;
                    msgBox.setText("Name must be unique, try again!");
                    msgBox.exec();
                }
                else break;
            }
            else
            {
                selectedEvent = NULL;
                ui->comboSelectEvent->setCurrentIndex(0);
                return;
            }
        }
        selectedEvent = NULL;
        Event *event = new Event();
        event->Name = text;
        ui->comboEventSignal->setCurrentIndex(0);
        event->Channel = ui->comboEventSignal->currentData().toString();
        event->Start = "0";
        event->Width = "10";
        event->Value = 0;
        ui->leEventName->setText(event->Name);
        ui->leEventStart->setText(event->Start);
        ui->leEventWidth->setText(event->Width);
        ui->leEventValue->setText(QString::number(event->Value));
        ui->leEventValueOff->setText(QString::number(event->ValueOff));
        Events.append(event);
        ui->comboSelectEvent->addItem(text);
        int i = (ui->comboSelectEvent->findText(text));
        ui->comboSelectEvent->setCurrentIndex(i);
        selectedEvent = event;
    }
    else if(ui->comboSelectEvent->currentText() == "Rename event")
    {
       if(!ui->leEventName->text().isEmpty())
       {
           while(true)
           {
               text = QInputDialog::getText(0, "Rename event", "Enter new event name, must be unique:", QLineEdit::Normal,"", &ok );
               if (ok && !text.isEmpty() )
               {
                   if(ui->comboSelectEvent->findText(text) >= 0)
                   {
                       QMessageBox msgBox;
                       msgBox.setText("Name must be unique, try again!");
                       msgBox.exec();
                   }
                   else break;
               }
               else return;
           }
           // Rename the event if here
           ui->comboSelectEvent->removeItem(ui->comboSelectEvent->findText(ui->leEventName->text()));
                   //ui->comboSelectEvent->findText(ui->leEventName->text());
           ui->comboSelectEvent->addItem(text);
           ui->leEventName->setText(text);
           slotEventUpdated();
           ui->comboSelectEvent->setCurrentIndex(0);
           ui->leEventName->setText("");
           ui->comboEventSignal->setCurrentIndex(0);
           ui->leEventStart->setText("");
           ui->leEventWidth->setText("");
           ui->leEventValue->setText("");
           ui->leEventValueOff->setText("");
           selectedEvent = NULL;
       }
    }
    else if(ui->comboSelectEvent->currentText() == "Delete current")
    {
        int   i;

        if((i=ui->comboSelectEvent->findText(ui->leEventName->text())) >= 3)
        {
            // Find in Event list and remove
            foreach(Event *evt, Events) if(evt->Name == ui->leEventName->text()) Events.removeOne(evt);
            ui->comboSelectEvent->removeItem(i);
            ui->comboSelectEvent->setCurrentIndex(0);
            ui->leEventName->setText("");
            ui->comboEventSignal->setCurrentIndex(0);
            ui->leEventStart->setText("");
            ui->leEventWidth->setText("");
            ui->leEventValue->setText("");
            ui->leEventValueOff->setText("");
            selectedEvent = NULL;
        }
    }
    else
    {
        // Find the event entry in list and update the controls
        selectedEvent = NULL;
        foreach(Event *evt, Events) if(evt->Name == ui->comboSelectEvent->currentText()) selectedEvent = evt;
        if(selectedEvent == NULL)
        {
            ui->leEventName->clear();
            ui->leEventStart->clear();
            ui->leEventWidth->clear();
            ui->leEventValue->clear();
            ui->leEventValueOff->clear();
            ui->comboEventSignal->setCurrentIndex(0);
            return;
        }
        ui->leEventName->setText(selectedEvent->Name);
        ui->leEventStart->setText(selectedEvent->Start);
        ui->leEventWidth->setText(selectedEvent->Width);
        ui->leEventValue->setText(QString::number(selectedEvent->Value));
        ui->leEventValueOff->setText(QString::number(selectedEvent->ValueOff));
        int i = ui->comboEventSignal->findData(selectedEvent->Channel);
        ui->comboEventSignal->setCurrentIndex(i);
    }
}

QString TimingGenerator::ProcessCommand(QString cmd)
{
    QLineEdit    *le    = NULL;
    QRadioButton *rb    = NULL;
    QComboBox    *combo = NULL;
    QPushButton  *pb    = NULL;
    QCheckBox    *chk   = NULL;

    if(!cmd.startsWith(Title)) return "?";
    if(cmd.trimmed() == Title + ".isTblMode")
    {
        if(isTableMode()) return("TRUE");
        else return("FALSE");
    }
    QStringList resList = cmd.split(",");
    if(resList[0].trimmed() == Title + ".Event.StartT")
    {
        for(int i=0;i<Events.count();i++)
        {
            if(Events[i]->Name == resList[1].trimmed()) return(QString::number(Events[i]->StartT));
        }
        return("?");
    }
    if(resList[0].trimmed() == Title + ".Event.Channel")
    {
        for(int i=0;i<Events.count();i++) if(Events[i]->Name == resList[1].trimmed()) return(Events[i]->Channel);
        return("?");
    }
    resList = cmd.split("=");
    if(resList[0].trimmed() == Title + ".Frame.Start") le = ui->leFrameStart;
    else if(resList[0].trimmed() == Title + ".Frame.Width") le = ui->leFrameWidth;
    else if(resList[0].trimmed() == Title + ".Frame.Accumulations") le = ui->leAccumulations;
    else if(resList[0].trimmed() == Title + ".Table") le = ui->leTable;
    else if(resList[0].trimmed() == Title + ".Event.Start") le = ui->leEventStart;
    else if(resList[0].trimmed() == Title + ".Event.Width") le = ui->leEventWidth;
    else if(resList[0].trimmed() == Title + ".Event.Value") le = ui->leEventValue;
    else if(resList[0].trimmed() == Title + ".Event.Value,off") le = ui->leEventValueOff;
    else if(resList[0].trimmed() == Title + ".Clock source") combo = ui->comboClockSource;
    else if(resList[0].trimmed() == Title + ".Trigger source") combo = ui->comboTriggerSource;
    else if(resList[0].trimmed() == Title + ".Mux order") combo = ui->comboMuxOrder;
    else if(resList[0].trimmed() == Title + ".Ext Clock Freq") le = ui->leExtClkFreq;
    else if(resList[0].trimmed() == Title + ".Time mode, in mS") chk = ui->chkTimeMode;
    else if(resList[0].trimmed() == Title + ".Frame.Enable") combo = ui->comboEnable;
    else if(resList[0].trimmed() == Title + ".Event.Signal") combo = ui->comboEventSignal;
    else if(resList[0].trimmed() == Title + ".Event.Select event") combo = ui->comboSelectEvent;
    else if(resList[0].trimmed() == Title + ".Generate") pb = ui->pbGenerate;
    else if(resList[0].trimmed() == Title + ".Trigger") pb = Trigger;
    else if(resList[0].trimmed() == Title + ".Abort") pb = Abort;
    else if(resList[0].trimmed() == Title + ".Edit") pb = Edit;
    if(le != NULL)
    {
       if(resList.count() == 1) return le->text();
       le->setText(resList[1]);
       le->setModified(true);
       le->editingFinished();
       return "";
    }
    if(rb != NULL)
    {
        if(resList.count() == 1) { if(rb->isChecked()) return "TRUE"; else return "FALSE"; }
        if(resList[1].trimmed() == "TRUE") rb->setChecked(true);
        if(resList[1].trimmed() == "FALSE") rb->setChecked(false);
        rb->clicked();
        return "";
    }
    if(chk != NULL)
    {
        if(resList.count() == 1) { if(chk->isChecked()) return "TRUE"; else return "FALSE"; }
        if(resList[1].trimmed() == "TRUE") chk->setChecked(true);
        if(resList[1].trimmed() == "FALSE") chk->setChecked(false);
        chk->clicked();
        return "";
    }
    if(combo != NULL)
    {
       if(resList.count() == 1) return combo->currentText();
       int i = combo->findText(resList[1].trimmed());
       if(i<0) return "?";
       combo->setCurrentIndex(i);
       return "";
    }
    if(pb != NULL)
    {
        pb->pressed();
        return "";
    }
    return "?";
}

void TimingGenerator::slotClearEvents(void)
{
   ui->pbClearEvents->setDown(false);
   Events.clear();
   ui->comboSelectEvent->clear();
   ui->comboSelectEvent->addItem("");
   ui->comboSelectEvent->addItem("New event");
   ui->comboSelectEvent->addItem("Rename event");
   ui->comboSelectEvent->addItem("Delete current");
}

void TimingGenerator::slotLoad(void)
{
    ui->pbLoad->setDown(false);
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Sequence from File"),"",tr("Sequence (*.seq);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to a QString
        QTextStream stream(&file);
        QString line;
        Events.clear();
        do
        {
            line = stream.readLine();
            SetValues(line);
        } while(!line.isNull());
    }
    file.close();
    ui->leEventName->clear();
    ui->leEventStart->clear();
    ui->leEventWidth->clear();
    ui->leEventValue->clear();
    ui->leEventValueOff->clear();
    ui->comboEventSignal->setCurrentIndex(0);
}

void TimingGenerator::slotSave(void)
{
    QString res;

    ui->pbSave->setDown(false);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Sequence File"),"",tr("Sequence (*.seq);;All files (*.*)"));
    if(fileName == "") return;
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Timing sequence, " + dateTime.toString() + "\n";
        stream << Report() + "\n";
        file.close();
    }
}

void TimingGenerator::slotClearTable(void)
{
    ui->leTable->clear();
}


// **********************************************************************************************
// IFT timing generation ************************************************************************
// **********************************************************************************************
IFTtiming::IFTtiming(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    properties = NULL;
    Enable = "";
    Grid1 = Grid3 = Grid2 = NULL;
    statusBar = NULL;
}

void IFTtiming::Show(void)
{
    // Make a group box
    gbIFT = new QGroupBox(Title,p);
    gbIFT->setGeometry(X,Y,500,180);
    gbIFT->setToolTip(MIPSnm + " IFT timing generation");
    // Place the controls on the group box
    leFillTime = new QLineEdit(gbIFT);    leFillTime->setGeometry(95,25,51,21);   leFillTime->setValidator(new QIntValidator); leFillTime->setText("50");
    leTrapTime = new QLineEdit(gbIFT);    leTrapTime->setGeometry(95,50,51,21);   leTrapTime->setValidator(new QIntValidator); leTrapTime->setText("10");
    leReleaseTime = new QLineEdit(gbIFT); leReleaseTime->setGeometry(95,75,51,21);leReleaseTime->setValidator(new QIntValidator); leReleaseTime->setText("5");
    leGrid1FillV = new QLineEdit(gbIFT);  leGrid1FillV->setGeometry(250,25,51,21); leGrid1FillV->setValidator(new QDoubleValidator); leGrid1FillV->setText("20");
    leGrid2ReleaseV = new QLineEdit(gbIFT);  leGrid2ReleaseV->setGeometry(250,50,51,21); leGrid2ReleaseV->setValidator(new QDoubleValidator); leGrid2ReleaseV->setText("30");
    leGrid3ReleaseV = new QLineEdit(gbIFT);  leGrid3ReleaseV->setGeometry(250,75,51,21); leGrid3ReleaseV->setValidator(new QDoubleValidator); leGrid3ReleaseV->setText("40");
    FrameSize = new QLineEdit(gbIFT);  FrameSize->setGeometry(405,25,51,21); FrameSize->setValidator(new QIntValidator); FrameSize->setText("1000");
    Accumulations = new QLineEdit(gbIFT); Accumulations->setGeometry(405,47,51,21); Accumulations->setValidator(new QIntValidator); Accumulations->setText("10");
    Table = new QLineEdit(gbIFT); Table->setGeometry(50,150,441,21);
    ClockSource = new QComboBox(gbIFT); ClockSource->setGeometry(400,69,100,21);
    ClockSource->clear();
    ClockSource->addItem("Ext","Ext");
    ClockSource->addItem("ExtN","ExtN");
    ClockSource->addItem("ExtS","ExtS");
    ClockSource->addItem("42000000","42000000");
    ClockSource->addItem("10500000","10500000");
    ClockSource->addItem("2625000","2625000");
    ClockSource->addItem("656250","656250");
    TriggerSource = new QComboBox(gbIFT); TriggerSource->setGeometry(400,91,100,21);
    TriggerSource->clear();
    TriggerSource->addItem("Software","Software");
    TriggerSource->addItem("Edge","Edge");
    TriggerSource->addItem("Pos","Pos");
    TriggerSource->addItem("Neg","Neg");
    GenerateTable = new QPushButton("Generate",gbIFT); GenerateTable->setGeometry(30,118,113,32); GenerateTable->setAutoDefault(false);
    Download = new QPushButton("Download",gbIFT);  Download->setGeometry(140,118,113,32); Download->setAutoDefault(false);
    Trigger = new QPushButton("Trigger",gbIFT);  Trigger->setGeometry(250,118,113,32); Trigger->setAutoDefault(false);
    Abort = new QPushButton("Abort",gbIFT);  Abort->setGeometry(360,118,113,32); Abort->setAutoDefault(false);
    // Add labels
    labels[0] = new QLabel("Fill time",gbIFT);       labels[0]->setGeometry(10,25,81,16);
    labels[1] = new QLabel("Trap time",gbIFT);       labels[1]->setGeometry(10,50,81,16);
    labels[2] = new QLabel("Release time",gbIFT);    labels[2]->setGeometry(10,75,81,16);
    labels[3] = new QLabel("Grid1 fill V",gbIFT);    labels[3]->setGeometry(160,25,80,16);
    labels[4] = new QLabel("Grid2 rel V",gbIFT);     labels[4]->setGeometry(160,50,80,16);
    labels[5] = new QLabel("Grid3 rel V",gbIFT);     labels[5]->setGeometry(160,75,80,16);
    labels[6] = new QLabel("Frame length",gbIFT);    labels[6]->setGeometry(310,25,91,16);
    labels[7] = new QLabel("Accumulations",gbIFT);   labels[7]->setGeometry(310,47,91,16);
    labels[8] = new QLabel("Clock source",gbIFT);    labels[8]->setGeometry(310,69,91,16);
    labels[9] = new QLabel("Trigger source",gbIFT);  labels[9]->setGeometry(310,91,91,16);
    labels[10] =new QLabel("Table",gbIFT);           labels[10]->setGeometry(10,150,59,16);
    // Connect to the event slots
    connect(GenerateTable,SIGNAL(pressed()),this,SLOT(pbGenerate()));
    connect(Download,SIGNAL(pressed()),this,SLOT(pbDownload()));
    connect(Trigger,SIGNAL(pressed()),this,SLOT(pbTrigger()));
    connect(Abort,SIGNAL(pressed()),this,SLOT(pbAbort()));
    connect(leFillTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leTrapTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leReleaseTime,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid1FillV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid2ReleaseV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(leGrid3ReleaseV,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(FrameSize,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    connect(Accumulations,SIGNAL(editingFinished()),this,SLOT(tblObsolite()));
    TableDownloaded = false;
    Table->raise();
    // Create the AcquireData object and init
    AD = new class AcquireData(this);
    AD->comms = comms;
    AD->statusBar = statusBar;
    AD->properties = properties;
    connect(AD,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
}

// Generate table for IFT.
//
// 0:[A:accumulations],
void IFTtiming::pbGenerate(void)
{
    QString table,msg;
    QString Grid1V, Grid2V, Grid3V;
    QMessageBox msgBox;
    int     cclock = 0;

    // Load the resting grid voltages from the setting, set to null if the grid is not defined
    Grid1V = "";
    if(Grid1 != NULL)
    {
        if(Grid1->Vsp->text().contains("?")) Grid1V = "0";
        else if(Grid1->Vsp->text()=="") Grid1V = "0";
        else Grid1V = Grid1->Vsp->text();
    }
    Grid2V = "";
    if(Grid2 != NULL)
    {
        if(Grid2->Vsp->text().contains("?")) Grid2V = "0";
        else if(Grid2->Vsp->text()=="") Grid2V = "0";
        else Grid2V = Grid2->Vsp->text();
    }
    Grid3V = "";
    if(Grid3 != NULL)
    {
        if(Grid3->Vsp->text().contains("?")) Grid3V = "0";
        else if(Grid3->Vsp->text()=="") Grid3V = "0";
        else Grid3V = Grid3->Vsp->text();
    }
    if((Grid2 == NULL) && (Grid3 == NULL))
    {
        msg =  "You must define Grid 2 or Grid 3 in the configuration file to use";
        msg += "the IFT function!";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    // Build the table
    table =  "STBLDAT;0:[A:" + QString::number(Accumulations->text().toInt() + 1) + ",";
    // Add fill time if grid 1 is defined and fill time is > 0
    if((Grid1 != NULL) && (leFillTime->text().toInt() > 0))
    {
        table += QString::number(cclock) + ":" + QString::number(Grid1->Channel) + ":" + leGrid1FillV->text() + ",";
        cclock += leFillTime->text().toInt();
    }
    // If trap time is > 0 then lower grid 1 and wait
    if(leTrapTime->text().toInt() > 0)
    {
        if(Grid1V != "") table += QString::number(cclock) + ":" + QString::number(Grid1->Channel) + ":" + Grid1V + ",";
        cclock += leTrapTime->text().toInt();
    }
    table += QString::number(cclock);
    if(Grid1V != "") table += ":" + QString::number(Grid1->Channel) + ":" + Grid1V;
    if(Grid2 != NULL) table += ":" + QString::number(Grid2->Channel) + ":" + leGrid2ReleaseV->text();
    if(Grid3 != NULL)table += ":" + QString::number(Grid3->Channel) + ":" + leGrid3ReleaseV->text();
    if(Enable != "") table += ":" + Enable + ":1";
    table += "," + QString::number(cclock + leReleaseTime->text().toInt());
    if(Grid2 != NULL) table += ":" + QString::number(Grid2->Channel) + ":" + Grid2V;
    if(Grid3 != NULL) table += ":" + QString::number(Grid3->Channel) + ":" + Grid3V;
    cclock += FrameSize->text().toInt();
    table += "," + QString::number(cclock) + ":" + Enable + ":0];";
    Table->setText(table);
}

// Download table parameters to MIPS
void IFTtiming::pbDownload(void)
{
   TableDownloaded = DownloadTable(comms, Table->text(), ClockSource->currentText(), TriggerSource->currentText());
}

void IFTtiming::pbTrigger(void)
{
    if(AD->isRunning())
    {
        if(properties != NULL) properties->Log("IFT trigger pressed while running!");
        return;
    }
    if(properties != NULL) properties->Log("IFT trigger pressed");
    if(properties != NULL)
    {
        if((properties->DataFilePath != "") && (properties->FileName != "")) AcquireData(properties->DataFilePath + "/" + properties->FileName);
        else AcquireData("");
    }
    else AcquireData("");
}

void IFTtiming::slotDataAcquired(QString filePath)
{
    emit dataAcquired(filePath);
}

void IFTtiming::AcquireData(QString path)
{
    AD->TableDownloaded = TableDownloaded;
    AD->StartAcquire(path, FrameSize->text().toInt(), Accumulations->text().toInt());
//    connect(AD,SIGNAL(dataAcquired(QString)),this,SLOT(slotDataAcquired(QString)));
}

// The function sends the table abort command to MIPS. Returns ACK if ok, else returns NAK.
void IFTtiming::pbAbort(void)
{
    if(comms == NULL) return;
    // Send table abort command
    if(comms->SendCommand("TBLABRT\n")) {if(statusBar != NULL) statusBar->showMessage("Table mode aborted!", 5000);}
    else if(statusBar != NULL) statusBar->showMessage("Table mode abort error!", 5000);
}

QString IFTtiming::Report(void)
{
    QString res;

    res = Title + ",";
    res += leFillTime->text() + ",";
    res += leTrapTime->text() + ",";
    res += leReleaseTime->text() + ",";
    res += leGrid1FillV->text() + ",";
    res += leGrid2ReleaseV->text() + ",";
    res += leGrid3ReleaseV->text() + ",";
    res += Accumulations->text() + ",";
    res += FrameSize->text() + ",";
    res += ClockSource->currentText() + ",";
    res += TriggerSource->currentText() + ",";
    res += Table->text() + "\n";
    return res;
}

bool IFTtiming::SetValues(QString strVals)
{
    QStringList resList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 12) return false;
    leFillTime->setText(resList[1]);
    leTrapTime->setText(resList[2]);
    leReleaseTime->setText(resList[3]);
    leGrid1FillV->setText(resList[4]);
    leGrid2ReleaseV->setText(resList[5]);
    leGrid3ReleaseV->setText(resList[6]);
    Accumulations->setText(resList[7]);
    FrameSize->setText(resList[8]);
    int i = ClockSource->findData(resList[9]);
    ClockSource->setCurrentIndex(i);
    i = TriggerSource->findData(resList[10]);
    TriggerSource->setCurrentIndex(i);
    i = strVals.indexOf("STBLDAT");
    if(i > 0) Table->setText(strVals.mid(i));
    pbDownload();
    return true;
}

void IFTtiming::tblObsolite(void)
{
    TableDownloaded = false;
}
