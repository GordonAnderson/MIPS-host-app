#include "dio.h"

DIO::DIO(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    dui->pbUPsmall->setText(QChar(0xB4, 0x25));
    dui->pbUPlarge->setText(QChar(0xB2, 0x25));
    dui->pbDOWNsmall->setText(QChar(0xBE, 0x25));
    dui->pbDOWNlarge->setText(QChar(0xBC, 0x25));
    QObjectList widgetList = dui->gbDigitalOut->children();
    widgetList += dui->gbDigitalIn->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            connect(((QCheckBox *)w),SIGNAL(stateChanged(int)),this,SLOT(DOUpdated()));
       }
    }
    connect(dui->pbDIOupdate,SIGNAL(pressed()),this,SLOT(UpdateDIO()));
    connect(dui->pbTrigHigh,SIGNAL(pressed()),this,SLOT(TrigHigh()));
    connect(dui->pbTrigLow,SIGNAL(pressed()),this,SLOT(TrigLow()));
    connect(dui->pbTrigPulse,SIGNAL(pressed()),this,SLOT(TrigPulse()));
    connect(dui->pbRFgenerate,SIGNAL(pressed()),this,SLOT(RFgenerate()));
    connect(dui->leSFREQ,SIGNAL(editingFinished()),this,SLOT(SetFreq()));
    connect(dui->leSWIDTH,SIGNAL(editingFinished()),this,SLOT(SetWidth()));
    connect(dui->chkRemoteNav,SIGNAL(stateChanged(int)),this,SLOT(RemoteNavigation()));
    connect(dui->pbUPsmall,SIGNAL(pressed()),this,SLOT(RemoteNavSmallUP()));
    connect(dui->pbUPlarge,SIGNAL(pressed()),this,SLOT(RemoteNavLargeUP()));
    connect(dui->pbDOWNsmall,SIGNAL(pressed()),this,SLOT(RemoteNavSmallDown()));
    connect(dui->pbDOWNlarge,SIGNAL(pressed()),this,SLOT(RemoteNavLargeDown()));
    connect(dui->pbSelect,SIGNAL(pressed()),this,SLOT(RemoteNavSelect()));
}

void DIO::Update(void)
{
    QString res;

    dui->tabMIPS->setEnabled(false);
    dui->statusBar->showMessage(tr("Updating Digital IO controls..."));
    QObjectList widgetList = dui->gbDigitalOut->children();
    widgetList += dui->gbFreqGen->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            res = "G" + w->objectName().mid(4).replace("_",",") + "\n";
            bool oldState = ((QCheckBox *)w)->blockSignals(true);
            if(comms->SendMess(res).toInt()==1) ((QCheckBox *)w)->setChecked(true);
            else ((QCheckBox *)w)->setChecked(false);
            ((QCheckBox *)w)->blockSignals(oldState);
       }
       if(w->objectName().contains("le"))
       {
            res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
            ((QLineEdit *)w)->setText(comms->SendMess(res));
       }
    }
    widgetList = dui->gbDigitalIn->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chk"))
       {
            res = "G" + w->objectName().mid(4).replace("_",",") + "\n";
            if(comms->SendMess(res).toInt()==1) ((QCheckBox *)w)->setChecked(true);
            else ((QCheckBox *)w)->setChecked(false);
       }
    }
    dui->tabMIPS->setEnabled(true);
    dui->statusBar->clearMessage();
}

void DIO::UpdateDIO(void)
{
    Update();
}

// Slot for Digital output check box selection
void DIO::DOUpdated(void)
{
    QObject* obj = sender();
    QString res;

    if(obj->objectName().startsWith("chkS"))
    {
        res = obj->objectName().mid(3).replace("_",",") + ",";
        if(((QCheckBox *)obj)->checkState()) res += "1\n";
        else res+= "0\n";
        comms->SendCommand(res.toStdString().c_str());
    }
}

// Slot for Trigger high pushbutton
void DIO::TrigHigh(void)
{
    comms->SendCommand("TRIGOUT,HIGH\n");
}

// Slot for Trigger low pushbutton
void DIO::TrigLow(void)
{
    comms->SendCommand("TRIGOUT,LOW\n");
}

// Slot for Trigger pulse pushbutton
void DIO::TrigPulse(void)
{
    comms->SendCommand("TRIGOUT,PULSE\n");
}

// Slot for RF generate pushbutton
void DIO::RFgenerate(void)
{
    QString res;

    res = "BURST," + dui->Burst->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
}

void DIO::SetFreq(void)
{
    QString res;

    if(!(dui->leSFREQ->isModified())) return;
    res = dui->leSFREQ->objectName().mid(2).replace("_",",") + "," + dui->leSFREQ->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    dui->leSFREQ->setModified(false);
}

void DIO::SetWidth(void)
{
    QString res;

    if(!(dui->leSWIDTH->isModified())) return;
    res = dui->leSWIDTH->objectName().mid(2).replace("_",",") + "," + dui->leSWIDTH->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    dui->leSWIDTH->setModified(false);
}


void DIO::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# DIO settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = dui->gbDigitalOut->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,4) == "chkS")
            {
                res = w->objectName() + ",";
                if(((QCheckBox *)w)->checkState()) res += "true\n";
                else res += "false\n";
                stream << res;
            }
        }
        file.close();
        dui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void DIO::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QObjectList widgetList = dui->gbDigitalOut->children();
    QFile file(Filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to the QString
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            resList = line.split(",");
            if(resList.count() == 2)
            {
                foreach(QObject *w, widgetList)
                {
                    if(w->objectName().mid(0,4) == "chkS")
                    {
                        if(w->objectName() == resList[0])
                        {
                            if(resList[1] == "true")
                            {
                                ((QCheckBox *)w)->setChecked(true);
                            }
                            else ((QCheckBox *)w)->setChecked(false);
                        }
                    }
                }

            }
        } while(!line.isNull());
        file.close();
        dui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}

// Following slot function support remote navication of UI
void DIO::RemoteNavigation(void)
{
   if(dui->chkRemoteNav->isChecked())
   {
       if(comms->SendCommand("SSERIALNAV,TRUE\n"))
       {
           dui->pbUPsmall->setEnabled(true);
           dui->pbUPlarge->setEnabled(true);
           dui->pbDOWNsmall->setEnabled(true);
           dui->pbDOWNlarge->setEnabled(true);
           dui->pbSelect->setEnabled(true);
           return;
       }
   }
   dui->pbUPsmall->setEnabled(false);
   dui->pbUPlarge->setEnabled(false);
   dui->pbDOWNsmall->setEnabled(false);
   dui->pbDOWNlarge->setEnabled(false);
   dui->pbSelect->setEnabled(false);
}

void DIO::RemoteNavSmallUP(void)
{
    char str[2];

    str[0] = 30;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavLargeUP(void)
{
    char str[2];

    str[0] = 31;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavSmallDown(void)
{
    char str[2];

    str[0] = 28;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavLargeDown(void)
{
    char str[2];

    str[0] = 29;
    str[1] = 0;
    comms->SendString(QString::fromStdString(str));
}

void DIO::RemoteNavSelect(void)
{
   char str[2];

   str[0] = 9;
   str[1] = 0;
   comms->SendString(QString::fromStdString(str));
}

// *************************************************************************************************
// DIO  ********************************************************************************************
// *************************************************************************************************

DIOchannel::DIOchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    Channel = "A";
    comms  = NULL;
    ReadOnly  = false;
}

void DIOchannel::Show(void)
{
    frmDIO = new QFrame(p); frmDIO->setGeometry(X,Y,170,21);
    DIO = new QCheckBox(frmDIO); DIO->setGeometry(0,0,170,21);
    DIO->setText(Title);
    DIO->setToolTip(MIPSnm + " DIO channel " + Channel);
    if(Channel > "P") ReadOnly = true;
    connect(DIO,SIGNAL(clicked(bool)),this,SLOT(DIOChange(bool)));
}

QString DIOchannel::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + ",";
    if(DIO->isChecked()) res += "1";
    else res += "0";
    return(res);
}

bool DIOchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList.count() < 2) return false;
    if(resList[1].contains("1")) DIO->setChecked(true);
    else DIO->setChecked(false);
    if(resList[1].contains("1")) DIO->stateChanged(1);
    else DIO->stateChanged(0);
    return true;
}

QString DIOchannel::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title)
    {
        if(DIO->isChecked()) return "1";
        return "0";
    }
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       if(resList[1] == "1") DIO->setChecked(true);
       else if(resList[1] == "0") DIO->setChecked(false);
       else return "?";
       if(resList[1] == "1") DIO->stateChanged(1);
       else  DIO->stateChanged(0);
       return "";
    }
    return "?";
}

void DIOchannel::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = comms->SendMess("GDIO," + Channel + "\n");
    bool oldState = DIO->blockSignals(true);
    if(res.contains("1")) DIO->setChecked(true);
    else if(res.contains("0")) DIO->setChecked(false);
    DIO->blockSignals(oldState);
}

void DIOchannel::DIOChange(bool  state)
{
   QString res;

   if(ReadOnly)
   {
      bool oldState = DIO->blockSignals(true);
      if(state) DIO->setChecked(false);
      else DIO->setChecked(true);
      DIO->blockSignals(oldState);
      return;
   }
   if(comms == NULL) return;
   if(DIO->checkState()) res ="SDIO," + Channel + ",1\n";
   else  res ="SDIO," + Channel + ",0\n";
   comms->SendCommand(res.toStdString().c_str());
}
