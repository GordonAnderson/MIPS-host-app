#include "rfdriver.h"

RFdriver::RFdriver(Ui::MIPS *w, Comms *c)
{
    rui = w;
    comms = c;

    SetNumberOfChannels(2);
    connect(rui->pbUpdateRF,SIGNAL(pressed()),this,SLOT(UpdateRFdriver()));
    connect(rui->leSRFFRQ,SIGNAL(editingFinished()),this,SLOT(leSRFFRQ_editingFinished()));
    connect(rui->leSRFDRV,SIGNAL(editingFinished()),this,SLOT(leSRFDRV_editingFinished()));
    connect(rui->pbAutoTune,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(rui->pbAutoRetune,SIGNAL(pressed()),this,SLOT(AutoRetune()));
    rui->leSRFFRQ->setValidator(new QIntValidator);
    rui->leSRFDRV->setValidator(new QDoubleValidator);
}

void RFdriver::SetNumberOfChannels(int num)
{
    disconnect(rui->comboRFchan, SIGNAL(currentTextChanged(QString)),0,0);
    NumChannels = num;
    rui->comboRFchan->clear();
    for(int i=0;i<NumChannels;i++)
    {
        rui->comboRFchan->addItem(QString::number(i+1));
    }
    connect(rui->comboRFchan,SIGNAL(currentTextChanged(QString)),this,SLOT(UpdateRFdriver()));
}

void RFdriver::Update(void)
{
    QString res;

    rui->tabMIPS->setEnabled(false);
    rui->statusBar->showMessage(tr("Updating RF driver controls..."));
    rui->leSRFFRQ->setText(comms->SendMess("GRFFRQ," + rui->comboRFchan->currentText() + "\n"));
    rui->leSRFDRV->setText(comms->SendMess("GRFDRV," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPPVP->setText(comms->SendMess("GRFPPVP," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPPVN->setText(comms->SendMess("GRFPPVN," + rui->comboRFchan->currentText() + "\n"));
    rui->leGRFPWR->setText(comms->SendMess("GRFPWR," + rui->comboRFchan->currentText() + "\n"));
    rui->tabMIPS->setEnabled(true);
    rui->statusBar->showMessage(tr(""));
}

// Select each channel from the tab to populate the group box
// and then save the group box parameters.
void RFdriver::Save(QString Filename)
{
    int i;
    int SelectedChannel = rui->comboRFchan->currentIndex();
    QString res;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# RFdriver settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = rui->gbRFparms->children();
        // Loop through all the channels
        for(i=0;i<NumChannels;i++)
        {
           rui->comboRFchan->setCurrentIndex(i);
           // Save the parameters to the file
           foreach(QObject *w, widgetList)
           {
               if(w->objectName().mid(0,3) == "leS")
               {
                   res = w->objectName() + "," + rui->comboRFchan->currentText() + "," + ((QLineEdit *)w)->text() + "\n";
                   stream << res;
               }
           }
        }
        rui->comboRFchan->setCurrentIndex(SelectedChannel);
        file.close();
        rui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

// Load all the parameters from the file, for the selected channel
// write to UI and update MIPS, for the other channels just update MIPS.
void RFdriver::Load(QString Filename)
{
    int SelectedChannel = rui->comboRFchan->currentIndex();
    QString res;
    QStringList resList;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    QObjectList widgetList = rui->gbRFparms->children();
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() == 3)
            {
                if(rui->comboRFchan->currentIndex() != (resList[1].toInt()-1)) rui->comboRFchan->setCurrentIndex(resList[1].toInt()-1);
                foreach(QObject *w, widgetList)
                {
                    if(w->objectName().mid(0,3) == "leS")
                    {
                        if(resList[2] != "") if(w->objectName() == resList[0])
                        {
                            ((QLineEdit *)w)->setText(resList[2]);
                            ((QLineEdit *)w)->setModified(true);
                            QMetaObject::invokeMethod(w, "editingFinished");
                        }
                    }
                }
            }
        } while(!line.isNull());
        rui->comboRFchan->setCurrentIndex(SelectedChannel);
        file.close();
        rui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}
void RFdriver::UpdateRFdriver(void)
{
    Update();
}

void RFdriver::leSRFFRQ_editingFinished()
{
    if(!rui->leSRFFRQ->isModified()) return;
    comms->SendCommand("SRFFRQ," + rui->comboRFchan->currentText() + "," + rui->leSRFFRQ->text() + "\n");
    rui->leSRFFRQ->setModified(false);
}

void RFdriver::leSRFDRV_editingFinished()
{
    if(!rui->leSRFDRV->isModified()) return;
    comms->SendCommand("SRFDRV," + rui->comboRFchan->currentText() + "," + rui->leSRFDRV->text() + "\n");
    rui->leSRFDRV->setModified(false);
}

void RFdriver::AutoTune(void)
{
    QMessageBox msgBox;

    rui->pbAutoTune->setDown(false);
    QString msg = "This function will tune the RF head attached to channel " + rui->comboRFchan->currentText() + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 3 minutes.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    if(!comms->SendCommand("TUNERFCH," + rui->comboRFchan->currentText() + "\n"))
    {
        QString msg = "Request failed!, could be a tune in process, only one channel ";
        msg += "can be tuned or retuned at a time. ";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }

}

void RFdriver::AutoRetune(void)
{
    QMessageBox msgBox;

    rui->pbAutoRetune->setDown(false);
    QString msg = "This function will retune the RF head attached to channel "  + rui->comboRFchan->currentText() + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 1 minute.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    if(!comms->SendCommand("RETUNERFCH," + rui->comboRFchan->currentText() + "\n"))
    {
        QString msg = "Request failed!, could be a tune in process, only one channel ";
        msg += "can be tuned or retuned at a time. ";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}

// *************************************************************************************************
// RF driver channel  ******************************************************************************
// *************************************************************************************************

RFchannel::RFchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
    qApp->installEventFilter(this);
    Updating = false;
    UpdateOff = false;
}

void RFchannel::Show(void)
{
    // Make a group box
    gbRF = new QGroupBox(Title,p);
    gbRF->setGeometry(X,Y,200,180);
    gbRF->setToolTip(MIPSnm + " RF channel " + QString::number(Channel));
    // Build and place the RF parameters group box
    Drive = new QLineEdit(gbRF); Drive->setGeometry(60,22,91,21);  Drive->setValidator(new QDoubleValidator);
    Freq = new QLineEdit(gbRF);  Freq->setGeometry(60,46,91,21);   Freq->setValidator(new QIntValidator);
    RFP = new QLineEdit(gbRF);   RFP->setGeometry(60,70,91,21);    RFP->setReadOnly(true);
    RFN = new QLineEdit(gbRF);   RFN->setGeometry(60,94,91,21);    RFN->setReadOnly(true);
    Power = new QLineEdit(gbRF); Power->setGeometry(60,118,91,21); Power->setReadOnly(true);
    // Place push buttons
    Tune = new QPushButton("Tune",gbRF);     Tune->setGeometry(10,147,81,32);    Tune->setAutoDefault(false);
    Retune = new QPushButton("Retune",gbRF); Retune->setGeometry(102,147,81,32); Retune->setAutoDefault(false);
    // Add labels
    labels[0] = new QLabel("Drive",gbRF); labels[0]->setGeometry(10,26,59,16);
    labels[1] = new QLabel("Freq",gbRF);  labels[1]->setGeometry(10,48,59,16);
    labels[2] = new QLabel("RF+",gbRF);   labels[2]->setGeometry(10,73,59,16);
    labels[3] = new QLabel("RF-",gbRF);   labels[3]->setGeometry(10,96,59,16);
    labels[4] = new QLabel("Power",gbRF); labels[4]->setGeometry(10,118,59,16);

    labels[5] = new QLabel("%",gbRF);     labels[5]->setGeometry(159,26,31,21);
    labels[6] = new QLabel("Hz",gbRF);    labels[6]->setGeometry(159,48,31,21);
    labels[7] = new QLabel("Vp-p",gbRF);  labels[7]->setGeometry(159,73,31,21);
    labels[8] = new QLabel("Vp-p",gbRF);  labels[8]->setGeometry(159,96,31,21);
    labels[9] = new QLabel("W",gbRF);     labels[9]->setGeometry(159,118,31,21);
    // Connect to the event slots
    connect(Drive,SIGNAL(editingFinished()),this,SLOT(DriveChange()));
    connect(Freq,SIGNAL(editingFinished()),this,SLOT(FreqChange()));
    connect(Tune,SIGNAL(pressed()),this,SLOT(TunePressed()));
    connect(Retune,SIGNAL(pressed()),this,SLOT(RetunePressed()));
}

bool RFchannel::eventFilter(QObject *obj, QEvent *event)
{
    float delta = 0;

    if (((obj == Drive) || (obj == Freq)) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           QString myString;
           if(obj == Drive)
           {
               myString.sprintf("%3.2f", Drive->text().toFloat() + delta);
               Drive->setText(myString);
               Drive->setModified(true);
               Drive->editingFinished();
               UpdateOff = false;
           }
           if(obj == Freq)
           {
               myString.sprintf("%5.0f", Freq->text().toFloat() + delta*1000);
               Freq->setText(myString);
               Freq->setModified(true);
               Freq->editingFinished();
               UpdateOff = false;
           }
           return true;
        }
    }
    UpdateOff = false;
    return QObject::eventFilter(obj, event);
}

// Returns a string with the following information
// Name,RFdrive,RFfreq,RFvolt+,RFvolt-,Power
QString RFchannel::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(isShutdown) res = title + "," + activeDrive;
    else res = title + "," + Drive->text();
    res += "," + Freq->text() + "," + RFP->text() + "," + RFN->text() + "," + Power->text();
    return(res);
}

// Sets the RFchannel parameters if the name matches and we have atleast 3 total parms
bool RFchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 3) return false;
    Freq->setText(resList[2]);
    Freq->setModified(true);
    Freq->editingFinished();
    if(isShutdown)
    {
        activeDrive = resList[1];
    }
    else
    {
        Drive->setText(resList[1]);
        Drive->setModified(true);
        Drive->editingFinished();
    }
    return true;
}

// The following commands are processed:
// title.Drive, read and set
// title.Freq, read and set
// title.RF+, read only
// title.RF-, read only
// title.Power, read only
// returns "?" if the command could not be processed
QString RFchannel::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title + ".Drive") return Drive->text();
    if(cmd == title + ".Freq") return Freq->text();
    if(cmd == title + ".RF+") return RFP->text();
    if(cmd == title + ".RF-") return RFN->text();
    if(cmd == title + ".Power") return Power->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
        if(cmd.startsWith(title + ".Drive"))
        {
            Drive->setText(resList[1]);
            Drive->setModified(true);
            Drive->editingFinished();
            return "";
        }
        if(cmd.startsWith(title + ".Freq"))
        {
            Freq->setText(resList[1]);
            Freq->setModified(true);
            Freq->editingFinished();
            return "";
        }
    }
    return "?";
}

// If sVals can contain up to five values, drive, freq, RF+, RF-, Power.
// This list is a comma seperated string. The values present are used
// and communications with MIPS is canceled.
// If its an empty string the MIPS is read for all the needed data.
void RFchannel::Update(QString sVals)
{
    QString     res;
    QStringList sValsList;
    bool        ok;

    if(sVals == "") sValsList.clear();
    else sValsList = sVals.split(",");
    if(comms == NULL) return;
    if(UpdateOff) return;
    Updating = true;
    comms->rb.clear();
    if(sValsList.count() < 2)
    {
      res = "GRFDRV,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[1];
    res.toFloat(&ok);
    if(!Drive->hasFocus() && (res != "") && ok) Drive->setText(res);
    if(sValsList.count() < 1)
    {
      res = "GRFFRQ,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[0];
    res.toInt(&ok);
    if(!Freq->hasFocus() && (res != "") && ok) Freq->setText(res);
    if(sValsList.count() < 3)
    {
      res = "GRFPPVP,"  + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[2];
    res.toFloat(&ok);
    if((res != "") && ok) RFP->setText(res);
    if(sValsList.count() < 4)
    {
      res = "GRFPPVN,"  + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[3];
    res.toFloat(&ok);
    if((res != "") && ok) RFN->setText(res);
    if(sValsList.count() < 5)
    {
      res = "GRFPWR,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[4];
    res.toFloat(&ok);
    if((res != "") && ok) Power->setText(res);
    Updating = false;
}

void RFchannel::DriveChange(void)
{
    if(comms == NULL) return;
    if(!Drive->isModified()) return;
    QString res = "SRFDRV," + QString::number(Channel) + "," + Drive->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Drive->setModified(false);
}

void RFchannel::FreqChange(void)
{
    if(comms == NULL) return;
    if(!Freq->isModified()) return;
    QString res = "SRFFRQ," + QString::number(Channel) + "," + Freq->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Freq->setModified(false);
}

void RFchannel::TunePressed(void)
{
    QMessageBox msgBox;

    Tune->setDown(false);
    QString msg = "This function will tune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 3 minutes.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Set drive to zero then delay
    QString res = "SRFDRV," + QString::number(Channel) + ",0\n";
    if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
    QTime timer;
    timer.start();
    while(timer.elapsed() < 1000) QApplication::processEvents();

    res = "TUNERFCH," + QString::number(Channel) + "\n";
    if(comms != NULL) comms->SendCommand(res);
}

void RFchannel::RetunePressed(void)
{
    QMessageBox msgBox;

    Retune->setDown(false);
    QString msg = "This function will retune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 1 minute.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    QString res = "RETUNERFCH," + QString::number(Channel) + "\n";
    if(comms != NULL) comms->SendCommand(res);
}

void RFchannel::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeDrive = Drive->text();
    Drive->setText("0");
    Drive->setModified(true);
    Drive->editingFinished();
}

void RFchannel::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    Drive->setText(activeDrive);
    Drive->setModified(true);
    Drive->editingFinished();
}

// *************************************************************************************************
// RF Closed loop driver channel  ******************************************************************
// *************************************************************************************************

RFCchannel::RFCchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
    qApp->installEventFilter(this);
    Updating = false;
    UpdateOff = false;
}

void RFCchannel::Show(void)
{
    // Make a group box
    gbRF = new QGroupBox(Title,p);
    gbRF->setGeometry(X,Y,200,225);
    gbRF->setToolTip(MIPSnm + " RF channel " + QString::number(Channel));
    // Build and place the RF parameters group box
    Drive = new QLineEdit(gbRF);    Drive->setGeometry(60,22,91,21);     Drive->setValidator(new QDoubleValidator);
    Setpoint = new QLineEdit(gbRF); Setpoint->setGeometry(60,46,91,21);  Setpoint->setValidator(new QDoubleValidator);
    Freq = new QLineEdit(gbRF);     Freq->setGeometry(60,70,91,21);      Freq->setValidator(new QIntValidator);
    RFP = new QLineEdit(gbRF);      RFP->setGeometry(60,94,91,21);       RFP->setReadOnly(true);
    RFN = new QLineEdit(gbRF);      RFN->setGeometry(60,118,91,21);      RFN->setReadOnly(true);
    Power = new QLineEdit(gbRF);    Power->setGeometry(60,142,91,21);    Power->setReadOnly(true);
    // Radio buttons for mode selection
    Open_Loop   = new QRadioButton("Open loop",gbRF);   Open_Loop->setGeometry(10,166,91,21);
    Closed_Loop = new QRadioButton("Close loop",gbRF); Closed_Loop->setGeometry(100,166,91,21);
    // Place push buttons
    Tune = new QPushButton("Tune",gbRF);     Tune->setGeometry(10,190,81,32);    Tune->setAutoDefault(false);
    Retune = new QPushButton("Retune",gbRF); Retune->setGeometry(102,190,81,32); Retune->setAutoDefault(false);
    // Add labels
    labels[0] = new QLabel("Drive",gbRF);    labels[0]->setGeometry(8,26,59,16);
    labels[1] = new QLabel("Setpoint",gbRF); labels[1]->setGeometry(8,48,59,16);
    labels[2] = new QLabel("Freq",gbRF);     labels[2]->setGeometry(8,73,59,16);
    labels[3] = new QLabel("RF+",gbRF);      labels[3]->setGeometry(8,96,59,16);
    labels[4] = new QLabel("RF-",gbRF);      labels[4]->setGeometry(8,118,59,16);
    labels[5] = new QLabel("Power",gbRF);    labels[5]->setGeometry(8,142,59,16);

    labels[6] = new QLabel("%",gbRF);     labels[6]->setGeometry(159,26,31,21);
    labels[7] = new QLabel("V",gbRF);     labels[7]->setGeometry(159,48,31,21);
    labels[8] = new QLabel("Hz",gbRF);    labels[8]->setGeometry(159,73,31,21);
    labels[9] = new QLabel("Vp-p",gbRF);  labels[9]->setGeometry(159,96,31,21);
    labels[10] = new QLabel("Vp-p",gbRF); labels[10]->setGeometry(159,118,31,21);
    labels[11] = new QLabel("W",gbRF);    labels[11]->setGeometry(159,142,31,21);
    // Connect to the event slots
    connect(Drive,SIGNAL(editingFinished()),this,SLOT(DriveChange()));
    connect(Setpoint,SIGNAL(editingFinished()),this,SLOT(SetpointChange()));
    connect(Freq,SIGNAL(editingFinished()),this,SLOT(FreqChange()));
    connect(Tune,SIGNAL(pressed()),this,SLOT(TunePressed()));
    connect(Retune,SIGNAL(pressed()),this,SLOT(RetunePressed()));
    connect(Open_Loop,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
    connect(Closed_Loop,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
}

bool RFCchannel::eventFilter(QObject *obj, QEvent *event)
{
    float delta = 0;

    if (((obj == Drive) || (obj == Freq)) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           QString myString;
           if(obj == Setpoint)
           {
               myString.sprintf("%3.2f", Setpoint->text().toFloat() + delta);
               Setpoint->setText(myString);
               Setpoint->setModified(true);
               Setpoint->editingFinished();
               UpdateOff = false;
           }
           if(obj == Drive)
           {
               myString.sprintf("%3.2f", Drive->text().toFloat() + delta);
               Drive->setText(myString);
               Drive->setModified(true);
               Drive->editingFinished();
               UpdateOff = false;
           }
           if(obj == Freq)
           {
               myString.sprintf("%5.0f", Freq->text().toFloat() + delta*1000);
               Freq->setText(myString);
               Freq->setModified(true);
               Freq->editingFinished();
               UpdateOff = false;
           }
           return true;
        }
    }
    UpdateOff = false;
    return QObject::eventFilter(obj, event);
}

// Returns a string with the following information
// Name,RFdrive,RFsetpoint,RFfreq,RFvolt+,RFvolt-,Power
QString RFCchannel::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(isShutdown) res = title + "," + activeDrive;
    else res = title + "," + Drive->text();
    res += "," + Freq->text() + "," + Setpoint->text() + "," + RFP->text() + "," + RFN->text() + "," + Power->text();
    return(res);
}

// Sets the RFchannel parameters if the name matches and we have at least 4 total parms
bool RFCchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 4) return false;
    Freq->setText(resList[3]);
    Freq->setModified(true);
    Freq->editingFinished();
    if(isShutdown)
    {
        activeDrive = resList[1];
        activeSetpoint = resList[2];
    }
    else
    {
        Drive->setText(resList[1]);
        Drive->setModified(true);
        Drive->editingFinished();
        Setpoint->setText(resList[2]);
        Setpoint->setModified(true);
        Setpoint->editingFinished();
    }
    return true;
}

// The following commands are processed:
// title.Drive, read and set
// title.Setpoint, read and set
// title.Freq, read and set
// title.RF+, read only
// title.RF-, read only
// title.Power, read only
// returns "?" if the command could not be processed
QString RFCchannel::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title + ".Drive") return Drive->text();
    if(cmd == title + ".Setpoint") return Setpoint->text();
    if(cmd == title + ".Freq") return Freq->text();
    if(cmd == title + ".RF+") return RFP->text();
    if(cmd == title + ".RF-") return RFN->text();
    if(cmd == title + ".Power") return Power->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
        if(cmd.startsWith(title + ".Drive"))
        {
            Drive->setText(resList[1]);
            Drive->setModified(true);
            Drive->editingFinished();
            return "";
        }
        if(cmd.startsWith(title + ".Setpoint"))
        {
            Setpoint->setText(resList[1]);
            Setpoint->setModified(true);
            Setpoint->editingFinished();
            return "";
        }
        if(cmd.startsWith(title + ".Freq"))
        {
            Freq->setText(resList[1]);
            Freq->setModified(true);
            Freq->editingFinished();
            return "";
        }
    }
    return "?";
}

// If sVals can contain up to five values, drive, freq, RF+, RF-, Power.
// This list is a comma seperated string. The values present are used
// and communications with MIPS is canceled.
// If its an empty string the MIPS is read for all the needed data.
// Setpoint value and open / closed loop must be read, never sent in sVals.
void RFCchannel::Update(QString sVals)
{
    QString     res;
    QStringList sValsList;
    bool        ok;

    if(sVals == "") sValsList.clear();
    else sValsList = sVals.split(",");
    if(comms == NULL) return;
    if(UpdateOff) return;
    Updating = true;
    comms->rb.clear();
    if(sValsList.count() < 2)
    {
      res = "GRFDRV,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[1];
    res.toFloat(&ok);
    if(!Drive->hasFocus() && (res != "") && ok) Drive->setText(res);
    if(sValsList.count() < 1)
    {
      res = "GRFFRQ,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[0];
    res.toInt(&ok);
    if(!Freq->hasFocus() && (res != "") && ok) Freq->setText(res);
    if(sValsList.count() < 3)
    {
      res = "GRFPPVP,"  + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[2];
    res.toFloat(&ok);
    if((res != "") && ok) RFP->setText(res);
    if(sValsList.count() < 4)
    {
      res = "GRFPPVN,"  + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[3];
    res.toFloat(&ok);
    if((res != "") && ok) RFN->setText(res);
    if(sValsList.count() < 5)
    {
      res = "GRFPWR,"   + QString::number(Channel) + "\n";
      res = comms->SendMess(res);
    }
    else res = sValsList[4];
    res.toFloat(&ok);
    if((res != "") && ok) Power->setText(res);
    // Update the setpoint and mode
    res = "GRFVLT,"   + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    res.toFloat(&ok);
    if(!Setpoint->hasFocus() && (res != "") && ok) Setpoint->setText(res);
    res = "GRFMODE," + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    if(res == "AUTO")
    {
        Closed_Loop->setChecked(true);
        Drive->setReadOnly(true);
        Setpoint->setReadOnly(false);
        Tune->setDisabled(true);
        Retune->setDisabled(true);
    }
    if(res == "MANUAL")
    {
        Open_Loop->setChecked(true);
        Drive->setReadOnly(false);
        Setpoint->setReadOnly(true);
        Tune->setDisabled(false);
        Retune->setDisabled(false);
    }
    Updating = false;
}

void RFCchannel::DriveChange(void)
{
    if(comms == NULL) return;
    if(!Drive->isModified()) return;
    QString res = "SRFDRV," + QString::number(Channel) + "," + Drive->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Drive->setModified(false);
}

void RFCchannel::SetpointChange(void)
{
    if(comms == NULL) return;
    if(!Setpoint->isModified()) return;
    QString res = "SRFVLT," + QString::number(Channel) + "," + Setpoint->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Setpoint->setModified(false);
}

void RFCchannel::FreqChange(void)
{
    if(comms == NULL) return;
    if(!Freq->isModified()) return;
    QString res = "SRFFRQ," + QString::number(Channel) + "," + Freq->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    Freq->setModified(false);
}

void RFCchannel::TunePressed(void)
{
    QMessageBox msgBox;

    Tune->setDown(false);
    QString msg = "This function will tune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 3 minutes.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    // Set drive to zero then delay
    QString res = "SRFDRV," + QString::number(Channel) + ",0\n";
    if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
    QTime timer;
    timer.start();
    while(timer.elapsed() < 1000) QApplication::processEvents();

    res = "TUNERFCH," + QString::number(Channel) + "\n";
    if(comms != NULL) comms->SendCommand(res);
}

void RFCchannel::RetunePressed(void)
{
    QMessageBox msgBox;

    Retune->setDown(false);
    QString msg = "This function will retune the RF head attached to " + MIPSnm + " channel " + QString::number(Channel) + ". ";
    msg += "Make sure the RF head is attached and connected to your system as needed. ";
    msg += "This process can take up to 1 minute.\n";
    msgBox.setText(msg);
    msgBox.setInformativeText("Are you sure you want to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return;
    QString res = "RETUNERFCH," + QString::number(Channel) + "\n";
    if(comms != NULL) comms->SendCommand(res);
}

void RFCchannel::rbChange(void)
{
    QString res = "SRFMODE,";
    if(Closed_Loop->isChecked())
    {
        res += QString::number(Channel) + ",AUTO\n";
        Drive->setReadOnly(true);
        Setpoint->setReadOnly(false);
        Tune->setDisabled(true);
        Retune->setDisabled(true);
    }
    if(Open_Loop->isChecked())
    {
        res += QString::number(Channel) + ",MANUAL\n";
        Drive->setReadOnly(false);
        Setpoint->setReadOnly(true);
        Tune->setDisabled(false);
        Retune->setDisabled(false);
    }
    if(comms == NULL) return;
    comms->SendCommand(res);
}

void RFCchannel::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeDrive = Drive->text();
    Drive->setText("0");
    Drive->setModified(true);
    Drive->editingFinished();
    activeSetpoint = Setpoint->text();
    Setpoint->setText("0");
    Setpoint->setModified(true);
    Setpoint->editingFinished();
}

void RFCchannel::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    Drive->setText(activeDrive);
    Drive->setModified(true);
    Drive->editingFinished();
    Setpoint->setText(activeSetpoint);
    Setpoint->setModified(true);
    Setpoint->editingFinished();
}

