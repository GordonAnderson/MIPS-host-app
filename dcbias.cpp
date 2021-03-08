#include "dcbias.h"

namespace Ui {
class MIPS;
}

DCbias::DCbias(Ui::MIPS *w, Comms *c)
{
    dui = w;
    comms = c;

    Updating = false;
    UpdateOff = false;
    SetNumberOfChannels(8);
    // DCbias page setup
    selectedLineEdit = NULL;
    QObjectList widgetList = dui->gbDCbias1->children();
    widgetList += dui->gbDCbias2->children();
    widgetList += dui->gbDCbias3->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leSDCB"))
       {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
           connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(DCbiasUpdated()));
       }
    }
    connect(dui->pbDCbiasUpdate,SIGNAL(pressed()),this,SLOT(UpdateDCbias()));
    connect(dui->chkPowerEnable,SIGNAL(toggled(bool)),this,SLOT(DCbiasPower()));
}

bool DCbias::myEventFilter(QObject *obj, QEvent *event)
{
    QLineEdit *le;
    float delta = 0;

   if (obj->objectName().startsWith("leSDCB_") && (event->type() == QEvent::KeyPress))
   {
       if(Updating) return true;
       UpdateOff = true;
       le = (QLineEdit *)obj;
       QKeyEvent *key = static_cast<QKeyEvent *>(event);
       if(key->key() == 16777235) delta = 0.1;
       if(key->key() == 16777237) delta = -0.1;
       if((QApplication::queryKeyboardModifiers() & 0xA000000) != 0) delta *= 0.1;
       if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
       if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
       // Get range for this channel
       int ch = obj->objectName().midRef(7).toInt();
       if(ch>24) ch = 25;
       else if(ch>16) ch = 17;
       else if(ch>8) ch = 9;
       else ch = 1;
       QLineEdit *lemin = dui->centralWidget->findChild<QLineEdit *>("leGDCMIN_" + QString::number(ch), Qt::FindChildrenRecursively);
       QLineEdit *lemax = dui->centralWidget->findChild<QLineEdit *>("leGDCMAX_" + QString::number(ch), Qt::FindChildrenRecursively);
       float min = lemin->text().toFloat();
       float max = lemax->text().toFloat();
       if((max-min) > 150) delta *= 10;
       if(delta != 0)
       {
          QString myString;
          myString.sprintf("%3.2f", le->text().toFloat() + delta);
          if(((le->text().toFloat() + delta) >= min) && ((le->text().toFloat() + delta) <= max))
          {
             le->setText(myString);
             le->setModified(true);
             emit le->editingFinished();
             return true;
          }
       }
   }
   return QObject::eventFilter(obj, event);
}

void DCbias::SetNumberOfChannels(int num)
{
    NumChannels = num;
    dui->gbDCbias1->setEnabled(false);
    dui->gbDCbias2->setEnabled(false);
    dui->gbDCbias3->setEnabled(false);
    if(NumChannels >= 8) dui->gbDCbias1->setEnabled(true);
    if(NumChannels > 8) dui->gbDCbias2->setEnabled(true);
    if(NumChannels > 16) dui->gbDCbias3->setEnabled(true);
}

// This function will search for all matching groups and apply
// the change to all channels in the same group.
void DCbias::ApplyDelta(QString GrpName, float change)
{
    QString  res;

   // Look through all the groups for matches
    for(int i=1;i<=24;i++)
    {
        res = "leGRP" + QString::number(i);
        QLineEdit *leGR = dui->gbDCbias1->findChild<QLineEdit *>(res);
        if(leGR != NULL)
        {
            if(leGR->text() == GrpName)
            {
                // Here if the name matches, now build the line edit box name
                res = "leSDCB_" + QString::number(i);
                QLineEdit *leDCB = dui->gbDCbias1->findChild<QLineEdit *>(res);
                if(leDCB != NULL)
                {
                    if(!leDCB->hasFocus())
                    {
                        // Read its value and change
                        leDCB->setText(QString::number(leDCB->text().toFloat() - change));
                        leDCB->setModified(true);
                        emit leDCB->editingFinished();
                    }
                }
            }
        }
    }
}

// This function is called when a DCbias value is changed. This function will send the command to
// MIPS to apply the new voltage.
void DCbias::DCbiasUpdated(void)
{
   QObject*    obj = sender();
   QString     res,ans;
   QStringList resList;

   if(Updating) return;
   if(!((QLineEdit *)obj)->isModified()) return;
   // If this channel is part of a group then first read the current value to calculate the
   // change. Apply the change to all channels in this group.
   // When we get here the change has alreay been made so we need to read the
   // current value from MIPS and calculate the difference.
   if((obj->objectName().startsWith("leSDCB_")) & (((QLineEdit *)obj)->hasFocus()))
   {
       resList = obj->objectName().split("_");
       res = "leGRP" + resList[1];
       QLineEdit *leGR = NULL;
       if(resList[1].toInt() <= 8) leGR = dui->gbDCbias1->findChild<QLineEdit *>(res);
       else if(resList[1].toInt() <= 16) leGR = dui->gbDCbias2->findChild<QLineEdit *>(res);
       else if(resList[1].toInt() <= 24) leGR = dui->gbDCbias3->findChild<QLineEdit *>(res);
       if(leGR != NULL)
       {
           if(leGR->text() != "")
           {
               // Here if this channel has a group label so we need to read the current
               // MIPS channel value to calculate the change
               res = "G" + obj->objectName().mid(3).replace("_",",") + "\n";
               //qDebug() << res;
               ans = comms->SendMess(res);
               //qDebug() << ans;
               // if(ans == "") ans="100";  // For testing
               if(ans != "")
               {
                   float oldvalue =ans.toFloat();
                   float change = oldvalue - ((QLineEdit *)obj)->text().toFloat();
                   // Now change all the values with the same group name
                   ApplyDelta(leGR->text(),change);
               }
           }
       }
   }
   res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
   UpdateOff = false;
}

void DCbias::Update(void)
{
    QString res;

    if(UpdateOff) return;
    Updating = true;
//    dui->tabMIPS->setEnabled(false);
//    dui->statusBar->showMessage(tr("Updating DC bias controls..."));
     // Read the number of channels and enable the proper controls
    dui->leGCHAN_DCB->setText(QString::number(NumChannels));
    res = comms->SendMess("GDCPWR\n");
    if(res == "ON") dui->chkPowerEnable->setChecked(true);
    if(res == "OFF") dui->chkPowerEnable->setChecked(false);
    QObjectList widgetList = dui->gbDCbias1->children();
    foreach(QObject *w, widgetList)
    {
       if( dui->tabMIPS->tabText(dui->tabMIPS->currentIndex()) != "DCbias") {Updating = false; return;}
       if((w->objectName().contains("le")) & (!w->objectName().contains("leGRP")))
       {
            if(!((QLineEdit *)w)->hasFocus())
            {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               res = comms->SendMess(res);
               if(res != "") ((QLineEdit *)w)->setText(res);
            }
       }
    }
    // Adjust range based on offet value
    dui->leGDCMIN_1->setText( QString::number(dui->leGDCMIN_1->text().toFloat()  + dui->leSDCBOF_1->text().toFloat()));
    dui->leGDCMAX_1->setText( QString::number(dui->leGDCMAX_1->text().toFloat()  + dui->leSDCBOF_1->text().toFloat()));
    if(NumChannels > 8)
    {
        QObjectList widgetList = dui->gbDCbias2->children();
        foreach(QObject *w, widgetList)
        {
           if( dui->tabMIPS->tabText(dui->tabMIPS->currentIndex()) != "DCbias") {Updating = false; return;}
           if((w->objectName().contains("le")) & (!w->objectName().contains("leGRP")))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  res = comms->SendMess(res);
                  if(res != "") ((QLineEdit *)w)->setText(res);
               }
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_9->setText( QString::number(dui->leGDCMIN_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
        dui->leGDCMAX_9->setText( QString::number(dui->leGDCMAX_9->text().toFloat()  + dui->leSDCBOF_9->text().toFloat()));
    }
    if(NumChannels > 16)
    {
        QObjectList widgetList = dui->gbDCbias3->children();
        foreach(QObject *w, widgetList)
        {
           if( dui->tabMIPS->tabText(dui->tabMIPS->currentIndex()) != "DCbias") {Updating = false; return;}
           if(w->objectName().contains("le"))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  res = comms->SendMess(res);
                  if(res != "") ((QLineEdit *)w)->setText(res);
               }
           }
        }
        // Adjust range based on offet value
        dui->leGDCMIN_17->setText( QString::number(dui->leGDCMIN_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
        dui->leGDCMAX_17->setText( QString::number(dui->leGDCMAX_17->text().toFloat()  + dui->leSDCBOF_17->text().toFloat()));
    }
//    dui->tabMIPS->setEnabled(true);
//    dui->statusBar->showMessage(tr(""));
    Updating = false;
}

void DCbias::DCbiasPower(void)
{
    if(dui->chkPowerEnable->isChecked()) comms->SendCommand("SDCPWR,ON\n");
    else comms->SendCommand("SDCPWR,OFF\n");
}

void DCbias::UpdateDCbias(void)
{
    Update();
}

void DCbias::Save(QString Filename)
{
    QString res;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# DCbias settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = dui->DCbias->children();
        widgetList += dui->gbDCbias1->children();
        if(NumChannels >8) widgetList += dui->gbDCbias2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName() == "chkPowerEnable")
            {
                stream << "chkPowerEnable,";
                if(dui->chkPowerEnable->isChecked()) stream << "ON\n";
                else stream << "OFF\n";
            }
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        file.close();
        dui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void DCbias::Load(QString Filename)
{
    QStringList resList;

    if(NumChannels == 0) return;
    if(Filename == "") return;
    QObjectList widgetList = dui->DCbias->children();
    widgetList += dui->gbDCbias1->children();
    if(NumChannels >8) widgetList += dui->gbDCbias2->children();
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
                    if(w->objectName().mid(0,3) == "leS")
                    {
                        if(resList[1] != "") if(w->objectName() == resList[0])
                        {
                            ((QLineEdit *)w)->setText(resList[1]);
                            ((QLineEdit *)w)->setModified(true);
                            QMetaObject::invokeMethod(w, "editingFinished");
                        }
                    }
                    if(w->objectName() == "chkPowerEnable")
                    {
                        if(w->objectName() == resList[0])
                        {
                            if(resList[1] == "ON")
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

// *************************************************************************************************
// DC bias channel  ********************************************************************************
// *************************************************************************************************

DCBchannel::DCBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
    Updating   = false;
    UpdateOff  = false;
    qApp->installEventFilter(this);
    DCBs.clear();
    LinkEnable = false;
    CurrentVsp = 0;
    UpdateCount = (qrand() & 3);
}

void DCBchannel::Show(void)
{
    frmDCB = new QFrame(p); frmDCB->setGeometry(X,Y,241,21);
    Vsp = new QLineEdit(frmDCB); Vsp->setGeometry(70,0,70,21); Vsp->setValidator(new QDoubleValidator);
    Vrb = new QLineEdit(frmDCB); Vrb->setGeometry(140,0,70,21); Vrb->setReadOnly(true);
    labels[0] = new QLabel(Title,frmDCB); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel("V",frmDCB);   labels[1]->setGeometry(220,0,21,16);
    connect(Vsp,SIGNAL(editingFinished()),this,SLOT(VspChange()));
    Vsp->setToolTip(MIPSnm + " channel " + QString::number(Channel));
}

bool DCBchannel::eventFilter(QObject *obj, QEvent *event)
{
    float delta = 0;

    if ((obj == Vsp) && (event->type() == QEvent::KeyPress))
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
           myString.sprintf("%3.2f", Vsp->text().toFloat() + delta);
           Vsp->setText(myString);
           Vsp->setModified(true);
           emit Vsp->editingFinished();
           UpdateOff = false;
           return true;
        }
    }
    UpdateOff = false;
    return QObject::eventFilter(obj, event);
}

QString DCBchannel::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(isShutdown) res = title + "," + activeVoltage + "," + Vrb->text();
    else res = Title + "," + Vsp->text() + "," + Vrb->text();
    return(res);
}

bool DCBchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList[0] != title) return false;
    if(resList.count() < 2) return false;
    if(isShutdown)
    {
        activeVoltage = resList[1];
        CurrentVsp = activeVoltage.toFloat();
    }
    else
    {
        Vsp->setText(resList[1]);
        CurrentVsp = Vsp->text().toFloat();
        Vsp->setModified(true);
        emit Vsp->editingFinished();
    }
    return true;
 }

// The following commands are processed:
// title            return the setpoint
// title=val        sets the setpoint
// title.readback   returns the readback voltage
// returns "?" if the command could not be processed
QString DCBchannel::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title) return Vsp->text();
    if(cmd == title + ".readback") return Vrb->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       Vsp->setText(resList[1].trimmed());
       Vsp->setModified(true);
       emit Vsp->editingFinished();
       return "";
    }
    return "?";
}

// If sVals can contain two values, Vsp,Vrb. If both values
// are present then MIPS is now read and the values updated.
// If only one value is present then its assumed to be Vsp.
// If its an empty string the MIPS is read for all the needed
// data.
void DCBchannel::Update(QString sVals)
{
    QString     res;
    QStringList sValsList;
    bool        ok;

    sValsList = sVals.split(",");
    if(comms == NULL) return;
    if(UpdateOff) return;
    Updating = true;
    comms->rb.clear();
    if(UpdateCount == 0) UpdateCount = 3;
    else UpdateCount--;
    if(sValsList.count() < 1)
    {
       res = "GDCB,"  + QString::number(Channel) + "\n";
       res = comms->SendMess(res);
       if(res == "")  // if true then the comms timed out
       {
          Updating = false;
          return;
       }
    }
    else res = sValsList[0];
    res.toFloat(&ok);
    if(!Vsp->hasFocus() && ok) Vsp->setText(res);
    CurrentVsp = Vsp->text().toFloat();
    if(sValsList.count() < 2)
    {
       res = "GDCBV," + QString::number(Channel) + "\n";
       res = comms->SendMess(res);
       if(res == "")  // if true then the comms timed out
       {
          Updating = false;
          return;
       }
    }
    else res = sValsList[1];
    res.toFloat(&ok);
    if(ok) Vrb->setText(res);
    // Compare setpoint with readback and color the readback background
    // depending on the difference.
    // No data = white
    if((Vsp->text()=="") | (Vrb->text()==""))
    {
        Vrb->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); }" );
        Updating = false;
        return;
    }
    float error = fabs(Vsp->text().toFloat() - Vrb->text().toFloat());
    if((error > 2.0) & (error > fabs(Vsp->text().toFloat()/100))) Vrb->setStyleSheet("QLineEdit { background: rgb(255, 204, 204); }" );
    else Vrb->setStyleSheet("QLineEdit { background: rgb(204, 255, 204); }" );
    //Vrb->setStyleSheet("QLineEdit { background: rgb(255, 204, 204); }" );  // light red
    //Vrb->setStyleSheet("QLineEdit { background: rgb(255, 255, 204); }" );  // light yellow
    //Vrb->setStyleSheet("QLineEdit { background: rgb(204, 255, 204); }" );  // light green
    Updating = false;
}

void DCBchannel::VspChange(void)
{
   //if(comms == NULL) return;
   if(!Vsp->isModified()) return;
   QString res = "SDCB," + QString::number(Channel) + "," + Vsp->text() + "\n";
   if(comms != NULL) comms->SendCommand(res.toStdString().c_str());
   // If this channel is part of a group calculate the delta and apply to all
   // other channels
   if((LinkEnable) && (DCBs.count()>0) && (CurrentVsp != Vsp->text().toFloat()))
   {
       float delta = CurrentVsp - Vsp->text().toFloat();
       foreach(DCBchannel * item, DCBs )
       {
           if(item != this)
           {
              item->CurrentVsp -= delta;
              item->Vsp->setText(QString::number(item->CurrentVsp,'f',2));
              item->CurrentVsp = item->Vsp->text().toFloat();
              item->Vsp->setModified(true);
              emit item->Vsp->editingFinished();
           }
       }
   }
   CurrentVsp = Vsp->text().toFloat();
   Vsp->setModified(false);
}

void DCBchannel::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeVoltage = Vsp->text();
    Vsp->setText("0");
    Vsp->setModified(true);
    emit Vsp->editingFinished();
}

void DCBchannel::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    Vsp->setText(activeVoltage);
    Vsp->setModified(true);
    emit Vsp->editingFinished();
}

// *************************************************************************************************
// DC bias offset  *********************************************************************************
// *************************************************************************************************

DCBoffset::DCBoffset(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
}

void DCBoffset::Show(void)
{
    frmDCBO = new QFrame(p); frmDCBO->setGeometry(X,Y,170,21);
    Voff = new QLineEdit(frmDCBO); Voff->setGeometry(70,0,70,21); Voff->setValidator(new QDoubleValidator);
    labels[0] = new QLabel(Title,frmDCBO); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel("V",frmDCBO);   labels[1]->setGeometry(150,0,21,16);
    Voff->setToolTip("Offset/range control " + MIPSnm);
    connect(Voff,SIGNAL(editingFinished()),this,SLOT(VoffChange()));
}

QString DCBoffset::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + "," + Voff->text();
    return(res);
}

bool DCBoffset::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList[0] != title) return false;
    if(resList.count() < 2) return false;
    Voff->setText(resList[1]);
    Voff->setModified(true);
    emit Voff->editingFinished();
    return true;
}

// The following commands are processed:
// title            return the offset value
// title=val        sets the offset value
// returns "?" if the command could not be processed
QString DCBoffset::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title) return Voff->text();
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       Voff->setText(resList[1]);
       Voff->setModified(true);
       emit Voff->editingFinished();
       return "";
    }
    return "?";
}

void DCBoffset::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = "GDCBOF,"  + QString::number(Channel) + "\n";
    res = comms->SendMess(res);
    if(res == "") return;
    if(!Voff->hasFocus()) Voff->setText(res);
}

void DCBoffset::VoffChange(void)
{
   if(comms == NULL) return;
   if(!Voff->isModified()) return;
   QString res = "SDCBOF," + QString::number(Channel) + "," + Voff->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   Voff->setModified(false);
}

// *************************************************************************************************
// DC bias enable  *********************************************************************************
// *************************************************************************************************

DCBenable::DCBenable(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    isShutdown = false;
}

void DCBenable::Show(void)
{
    frmDCBena = new QFrame(p); frmDCBena->setGeometry(X,Y,170,21);
    DCBena = new QCheckBox(frmDCBena); DCBena->setGeometry(0,0,170,21);
    DCBena->setText(Title);
    DCBena->setToolTip("Enables all DC bias channels on " + MIPSnm);
    connect(DCBena,SIGNAL(stateChanged(int)),this,SLOT(DCBenaChange()));
}

QString DCBenable::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + ",";
    if(isShutdown)
    {
        if(activeEnableState) res += "ON";
        else res += "OFF";
    }
    else
    {
        if(DCBena->isChecked()) res += "ON";
        else res += "OFF";
    }
    return(res);
}

bool DCBenable::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList[0] != title) return false;
    if(resList.count() < 2) return false;
    if(isShutdown)
    {
        if(resList[1].contains("ON")) activeEnableState = true;
        else activeEnableState = false;
        return true;
    }
    if(resList[1].contains("ON")) DCBena->setChecked(true);
    else DCBena->setChecked(false);
    if(resList[1].contains("ON")) emit DCBena->stateChanged(1);
    else  emit DCBena->stateChanged(0);
    return true;
}

// The following commands are processed:
// title            return the status, ON or OFF
// title=val        sets the status, ON or OFF
// returns "?" if the command could not be processed
QString DCBenable::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title)
    {
        if(DCBena->isChecked()) return "ON";
        return "OFF";
    }
    QStringList resList = cmd.split("=");
    if(resList.count()==2)
    {
       if(resList[1] == "ON") DCBena->setChecked(true);
       else if(resList[1] == "OFF") DCBena->setChecked(false);
       else return "?";
       if(resList[1] == "ON") emit DCBena->stateChanged(1);
       else  emit DCBena->stateChanged(0);
       return "";
    }
    return "?";
}

void DCBenable::Update(void)
{
    QString res;

    if(comms == NULL) return;
    comms->rb.clear();
    res = comms->SendMess("GDCPWR\n");
    bool oldState = DCBena->blockSignals(true);
    if(res.contains("ON")) DCBena->setChecked(true);
    if(res.contains("OFF")) DCBena->setChecked(false);
    DCBena->blockSignals(oldState);
}

void DCBenable::DCBenaChange(void)
{
   QString res;

   if(comms == NULL) return;
   if(DCBena->checkState()) res ="SDCPWR,ON\n";
   else res ="SDCPWR,OFF\n";
   comms->SendCommand(res.toStdString().c_str());
}

void DCBenable::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeEnableState = DCBena->checkState();
    DCBena->setChecked(false);
    emit DCBena->stateChanged(0);
}

void DCBenable::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    DCBena->setChecked(activeEnableState);
    emit DCBena->stateChanged(0);
}
