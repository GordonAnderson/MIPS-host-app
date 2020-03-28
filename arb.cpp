#include "arb.h"
#include "help.h"

#include <QProcess>
#include <qthread.h>

namespace Ui {
class MIPS;
}

ARB::ARB(Ui::MIPS *w, Comms *c)
{
    aui = w;
    comms = c;

    LogedData = new Help();
//NumChannels = 32;
    Compressor = true;
    PPP = 32;
    // Setup the module selection combo box
    aui->comboARBmodule->clear();
    for(int i=0;i<4;i++)
    {
        if(((i+1) * 8) <= NumChannels) aui->comboARBmodule->addItem(QString::number(i+1));
    }
    // Setup the waveform type combo box
    aui->comboSWFTYP->clear();
    aui->comboSWFTYP->addItem("SIN","SIN");
    aui->comboSWFTYP->addItem("RAMP","RAMP");
    aui->comboSWFTYP->addItem("TRI","TRI");
    aui->comboSWFTYP->addItem("PULSE","PULSE");
    aui->comboSWFTYP->addItem("ARB","ARB");
    QObjectList widgetList = aui->gbARBmodule1->children();
    widgetList += aui->gbARBmodule2->children();
    widgetList += aui->gbARBcompressor->children();
    widgetList += aui->gbARBtiming->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS"))
       {
            //((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(ARBUpdated()));
       }
    }
    widgetList = aui->gbARBtwaveParms->children();
    widgetList += aui->gbDualOutput->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS"))
       {
            //((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(ARBUpdatedParms()));
       }
    }
    connect(aui->pbSetChannel,SIGNAL(pressed()),this,SLOT(SetARBchannel()));
    connect(aui->pbSetRangeChannel,SIGNAL(pressed()),this,SLOT(SetARBchannelRange()));
    connect(aui->pbARBtrigger,SIGNAL(pressed()),this,SLOT(ARBtrigger()));
    connect(aui->pbSetChannel_2,SIGNAL(pressed()),this,SLOT(SetARBchannel_2()));
    connect(aui->pbSetRangeChannel_2,SIGNAL(pressed()),this,SLOT(SetARBchannelRange_2()));
    connect(aui->pbARBtrigger_2,SIGNAL(pressed()),this,SLOT(ARBtrigger_2()));
    connect(aui->pbARBviewLog,SIGNAL(pressed()),this,SLOT(ARBviewLog()));
    connect(aui->pbARBclearLog,SIGNAL(pressed()),this,SLOT(ARBclearLog()));
    connect(aui->pbARBupdate,SIGNAL(pressed()),this,SLOT(ARBupdate()));
    connect(aui->pbARBtwaveUpdate,SIGNAL(pressed()),this,SLOT(ARBupdate()));
    connect(aui->pbEditARBwf,SIGNAL(pressed()),this,SLOT(EditARBwaveform()));
    connect(aui->tabARB,SIGNAL(currentChanged(int)),this,SLOT(ARBtabSelected()));
    connect(aui->comboSWFTYP,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBtypeSelected()));
    connect(aui->comboARBmodule,SIGNAL(currentIndexChanged(int)),this,SLOT(ARBmoduleSelected()));
    connect(aui->rbSWFDIR_FWD,SIGNAL(clicked(bool)),this,SLOT(rbTWfwd()));
    connect(aui->rbSWFDIR_REV,SIGNAL(clicked(bool)),this,SLOT(rbTWrev()));
    // Compressor controls
    connect(aui->rbSARBCMODE_COMPRESS,SIGNAL(clicked(bool)),this,SLOT(rbModeCompress()));
    connect(aui->rbSARBCMODE_NORMAL,SIGNAL(clicked(bool)),this,SLOT(rbModeNormal()));
    connect(aui->rbSARBCSW_CLOSE,SIGNAL(clicked(bool)),this,SLOT(rbSwitchClose()));
    connect(aui->rbSARBCSW_OPEN,SIGNAL(clicked(bool)),this,SLOT(rbSwitchOpen()));
    connect(aui->pbARBforceTrigger,SIGNAL(pressed()),this,SLOT(pbForceTrigger()));
}

void ARB::SetNumberOfChannels(int num)
{
    NumChannels = num;
    aui->comboARBmodule->clear();
    for(int i=0;i<4;i++)
    {
        if(((i+1) * 8) <= NumChannels) aui->comboARBmodule->addItem(QString::number(i+1));
    }
}

void ARB::pbForceTrigger(void)
{
   comms->SendCommand("TARBTRG\n");
}

void ARB::rbModeCompress(void)
{
    comms->SendCommand("SARBCMODE,Compress\n");
}

void ARB::rbModeNormal(void)
{
    comms->SendCommand("SARBCMODE,Normal\n");
}

void ARB::rbSwitchClose(void)
{
   comms->SendCommand("SARBCSW,Close\n");
}

void ARB::rbSwitchOpen(void)
{
   comms->SendCommand("SARBCSW,Open\n");
}

void ARB::rbTWfwd(void)
{
    comms->SendCommand("SWFDIR,1,FWD\n");
}

void ARB::rbTWrev(void)
{
    comms->SendCommand("SWFDIR,1,REV\n");
}

void ARB::ARBtypeSelected(void)
{
    comms->SendCommand("SWFTYP," + aui->comboARBmodule->currentText() + "," + aui->comboSWFTYP->currentText() + "\n");
}

void ARB::ARBtrigger(void)
{
    comms->SendCommand("SWFDIS,1\n");
    comms->SendCommand("SWFENA,1\n");
}
void ARB::ARBtrigger_2(void)
{
    comms->SendCommand("SWFDIS,2\n");
    comms->SendCommand("SWFENA,2\n");
}

void ARB::ARBtabSelected(void)
{
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "ARB mode")
    {
        if(NumChannels > 0) comms->SendCommand("SARBMODE,1,ARB\n");
        if(NumChannels > 8) comms->SendCommand("SARBMODE,2,ARB\n");
        QThread::msleep(2000);
     }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "Twave mode")
    {
        if(NumChannels > 0) comms->SendCommand("SARBMODE,1,TWAVE\n");
        if(NumChannels > 8) comms->SendCommand("SARBMODE,2,TWAVE\n");
        QThread::msleep(2000);
    }
    Update();
}

void ARB::SetARBchannel(void)
{
    QString res;

    res = aui->leChan->text();
    if(res.toInt() == 0)
    {
        res = "SARBCHS,1," + aui->leLevel->text() + "\n";
        LogString += res;
        comms->SendCommand(res.toStdString().c_str());
        return;
    }
    res = "SARBCH,1," + aui->leChan->text() + "," + aui->leLevel->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}
void ARB::SetARBchannel_2(void)
{
    QString res;

    res = aui->leChan->text();
    if(res.toInt() == 0)
    {
        res = "SARBCHS,2," + aui->leLevel_2->text() + "\n";
        LogString += res;
        comms->SendCommand(res.toStdString().c_str());
        return;
    }
    res = "SARBCH,2," + aui->leChan_2->text() + "," + aui->leLevel_2->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}

void ARB::SetARBchannelRange(void)
{
    QString res;

    res = "SACHRNG,1," + aui->leRangeChan->text() + "," + aui->leRangeStart->text() + "," + aui->leRangeStop->text() + "," + aui->leRangeLevel->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}
void ARB::SetARBchannelRange_2(void)
{
    QString res;

    res = "SACHRNG,2," + aui->leRangeChan_2->text() + "," + aui->leRangeStart_2->text() + "," + aui->leRangeStop_2->text() + "," + aui->leRangeLevel_2->text() + "\n";
    LogString += res;
    comms->SendCommand(res.toStdString().c_str());
}

void ARB::ARBmoduleSelected(void)
{
    QString res;

    if( aui->tabMIPS->tabText(aui->tabMIPS->currentIndex()) != "ARB") return;
     QObjectList widgetList = aui->gbARBtwaveParms->children();
    QString chan = aui->comboARBmodule->currentText();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
           res = "G" + w->objectName().mid(3) + "," + chan;
           ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
       }
    }
    res = comms->SendMess("GWFDIR," + chan +"\n");
    if(res == "FWD") aui->rbSWFDIR_FWD->setChecked(true);
    if(res == "REV") aui->rbSWFDIR_REV->setChecked(true);
    res = comms->SendMess("GWFTYP," + chan + "\n");
    int i = aui->comboSWFTYP->findData(res);
    aui->comboSWFTYP->setCurrentIndex(i);
}

void ARB::ARBUpdated(void)
{
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2).replace("_",",");
   if(res.right(1) == ",") res = res.left(res.length()-1);
   res += "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void ARB::ARBUpdatedParms(void)
{
   QString chan = aui->comboARBmodule->currentText();
   QObject* obj = sender();
   QString res;

   if(!((QLineEdit *)obj)->isModified()) return;
   res = obj->objectName().mid(2);
   res += "," + chan + "," + ((QLineEdit *)obj)->text() + "\n";
   comms->SendCommand(res.toStdString().c_str());
   ((QLineEdit *)obj)->setModified(false);
}

void ARB::ARBupdate(void)
{
    Update();
}

void ARB::Update(void)
{
    QString res;
    QObjectList widgetList;

    // Exit if not selected
    if( aui->tabMIPS->tabText(aui->tabMIPS->currentIndex()) != "ARB") return;
    // Determine the number of ARB channels and exit if its zero
    res = comms->SendMess("GCHAN,ARB\n");
    NumChannels = res.toInt();
    if(NumChannels == 0)
    {
        aui->gbARBmodule1->setEnabled(false);
        aui->gbARBmodule2->setEnabled(false);
        return;
    }
    aui->gbARBmodule1->setEnabled(true);
    if(NumChannels > 8) aui->gbARBmodule2->setEnabled(true);
    // Get the first modules's mode and set the tab to that mode and
    // if there are two modules set second to match first
    res = comms->SendMess("GARBMODE,1\n");
    if(res == "ARB")
    {
        aui->tabARB->setCurrentIndex(1);
    }
    if(res == "TWAVE")
    {
        aui->tabARB->setCurrentIndex(0);
    }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "ARB mode")
    {
        QObjectList widgetList = aui->gbARBmodule1->children();
        if(NumChannels > 8) widgetList += aui->gbARBmodule2->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().contains("le"))
           {
               res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
               ((QLineEdit *)w)->setText(comms->SendMess(res));
           }
        }
    }
    if(aui->tabARB->tabText(aui->tabARB->currentIndex()) == "Twave mode")
    {
        if(NumChannels >= 16)
        {
            if(Compressor) aui->gbARBcompressor->setEnabled(true);
            else aui->gbARBcompressor->setEnabled(false);
        }
        else aui->gbARBcompressor->setEnabled(false);
        if(Compressor)
        {
           widgetList = aui->gbARBcompressor->children();
           widgetList += aui->gbARBtiming->children();
           foreach(QObject *w, widgetList)
           {
              if(w->objectName().contains("le"))
              {
                  res = "G" + w->objectName().mid(3).replace("_",",");
                  if(res.right(1) == ",") res = res.left(res.length()-1);
                  ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
              }
           }
        }
        // Check for dual output boards and individual offset control
        QString chan = aui->comboARBmodule->currentText();
        if(comms->SendMess("GARBOFFA," + chan + "\n").contains("?")) aui->gbDualOutput->setEnabled(false);
        else aui->gbDualOutput->setEnabled(true);
        widgetList = aui->gbARBtwaveParms->children();
        if(aui->gbDualOutput->isEnabled()) widgetList += aui->gbDualOutput->children();
        foreach(QObject *w, widgetList)
        {
           if(w->objectName().startsWith("le"))
           {
               res = "G" + w->objectName().mid(3) + "," + chan;
               ((QLineEdit *)w)->setText(comms->SendMess(res + "\n"));
           }
        }
        res = comms->SendMess("GWFDIR," + chan +"\n");
        if(res == "FWD") aui->rbSWFDIR_FWD->setChecked(true);
        if(res == "REV") aui->rbSWFDIR_REV->setChecked(true);
        res = comms->SendMess("GWFTYP," + chan + "\n");
        int i = aui->comboSWFTYP->findData(res);
        aui->comboSWFTYP->setCurrentIndex(i);
     }
     if(Compressor)
     {
         res = comms->SendMess("GARBCMODE\n");
         if(res == "Normal") aui->rbSARBCMODE_NORMAL->setChecked(true);
         if(res == "Compress") aui->rbSARBCMODE_COMPRESS->setChecked(true);
         res = comms->SendMess("GARBCSW\n");
         if(res == "Open") aui->rbSARBCSW_OPEN->setChecked(true);
         if(res == "Close") aui->rbSARBCSW_CLOSE->setChecked(true);
     }
}


void ARB::ARBclearLog(void)
{
    LogString = "";
}

void ARB::ARBviewLog(void)
{
    LogedData->SetTitle("Log of commands sent to MIPS");
    LogedData->LoadStr(LogString);
    LogedData->show();
}

void ARB::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# ARB settings, " + dateTime.toString() + "\n";
        // Loop thru all the channels and save ARB parameters
        QObjectList widgetList = aui->gbARBtwaveParms->children();
        for(int i=0;i<aui->comboARBmodule->count();i++)
        {
            aui->comboARBmodule->setCurrentIndex(i);
            // likely need to update the ARB parameters on the dialog here
            QApplication::processEvents();
            foreach(QObject *w, widgetList)
            {
                if(w->objectName().mid(0,3) == "leS")
                {
                    res = w->objectName().mid(2) + "," + aui->comboARBmodule->currentText() + "," + ((QLineEdit *)w)->text() + "\n";
                    stream << res;
                }
            }
            // Save the waveform direction and type
            if(aui->rbSWFDIR_FWD->isChecked()) res = "SWFDIR," + aui->comboARBmodule->currentText() + ",FWD\n";
            if(aui->rbSWFDIR_REV->isChecked()) res = "SWFDIR," + aui->comboARBmodule->currentText() + ",REV\n";
            stream << res;
            res = "SWFTYP," + aui->comboARBmodule->currentText() + "," + aui->comboSWFTYP->currentText() + "\n";
            stream << res;
        }
        widgetList = aui->gbARBcompressor->children();
        widgetList += aui->gbARBmodule1->children();
        if(NumChannels >8) widgetList += aui->gbARBmodule2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        stream << "\n";
        stream << LogString;
        file.close();
        aui->statusBar->showMessage("Settings saved to " + Filename,2000);
    }
}

void ARB::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
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
            if(line.trimmed().mid(0,1) != "#") comms->SendCommand(line + "\n");
            QApplication::processEvents();
        } while(!line.isNull());
        file.close();
        Update();
        aui->statusBar->showMessage("Settings loaded from " + Filename,2000);
    }
}

void ARB::ReadWaveform(void)
{
    int Waveform[32];
    QString cmd;
    int i;

    // Read waveform
    ARBwfEdit->GetWaveform(Waveform);
    // Send waveform to MIPS
    cmd = "SWFARB,"  + aui->comboARBmodule->currentText();
    for(i=0; i<PPP; i++) cmd += "," + QString::number(Waveform[i]);
    cmd += "\n";
    if(!comms->SendCommand(cmd))
    {
        aui->statusBar->showMessage("Error sending waveform to MIPS",2000);
    }
}

void ARB::EditARBwaveform(void)
{
   int     Waveform[32];
   int     i;
   QString cmd,res;

   for(i=0; i<32; i++) Waveform[i] = i*4 - 64;
   // Determine the PPP before we popup the edit dialog
   if(comms != NULL)
   {
       cmd =  "GARBPPP,"  + aui->comboARBmodule->currentText();
       res = comms->SendMess(cmd + "\n");
       if(!res.isEmpty())
       {
           PPP = res.toInt();
           if(PPP < 8) PPP = 8;
           if(PPP > 32) PPP = 32;
       }
   }
   // Read the ARB waveform from MIPS
   res = comms->SendMess("GWFARB," + aui->comboARBmodule->currentText() + "\n");
   if(res.contains("?"))
   {
       // Here if the message was NAKed
       aui->statusBar->showMessage("Error reading waveform from MIPS",2000);
       return;
   }
   QStringList Vals = res.split(",");
   for(i=0; i<PPP; i++)
   {
       if(i < Vals.count()) Waveform[i] = Vals[i].toInt();
       else Waveform[i] = 0;
   }
   ARBwfEdit = new ARBwaveformEdit(0,PPP);
   connect(ARBwfEdit, SIGNAL(WaveformReady()), this, SLOT(ReadWaveform()));
   ARBwfEdit->SetWaveform(Waveform);
   ARBwfEdit->show();
}

// **********************************************************************************************
// ARB ******************************************************************************************
// **********************************************************************************************

ARBchannel::ARBchannel(QWidget *parent, QString name, QString MIPSname, int x, int y) : QWidget(parent)
{
    p      = parent;
    Title  = name;
    MIPSnm = MIPSname;
    X      = x;
    Y      = y;
    comms  = NULL;
    statusBar = NULL;
    PPP = 32;
    isShutdown = false;
    Updating = false;
    UpdateOff = false;
    qApp->installEventFilter(this);
}

void ARBchannel::Show(void)
{
    // Make a group box
    gbARB = new QGroupBox(Title,p);
    gbARB->setGeometry(X,Y,250,200);
    gbARB->setToolTip(MIPSnm + " ARB channel " + QString::number(Channel));
    // Place the controls on the group box
    leSWFREQ = new QLineEdit(gbARB);   leSWFREQ->setGeometry(100,22,91,21);  leSWFREQ->setValidator(new QIntValidator);
    leSWFVRNG = new QLineEdit(gbARB);  leSWFVRNG->setGeometry(100,46,91,21); leSWFVRNG->setValidator(new QDoubleValidator);
    leSWFVAUX = new QLineEdit(gbARB);  leSWFVAUX->setGeometry(100,70,91,21); leSWFVAUX->setValidator(new QDoubleValidator);
    leSWFVOFF = new QLineEdit(gbARB);  leSWFVOFF->setGeometry(100,94,91,21); leSWFVOFF->setValidator(new QDoubleValidator);
    Waveform = new QComboBox(gbARB);   Waveform->setGeometry(100,118,91,21);
    Waveform->clear();
    Waveform->addItem("SIN","SIN");
    Waveform->addItem("RAMP","RAMP");
    Waveform->addItem("TRI","TRI");
    Waveform->addItem("PULSE","PULSE");
    Waveform->addItem("ARB","ARB");
    EditWaveform = new QPushButton("Edit",gbARB);     EditWaveform->setGeometry(100,142,91,30); EditWaveform->setAutoDefault(false);
    SWFDIR_FWD   = new QRadioButton("Forward",gbARB); SWFDIR_FWD->setGeometry(20,166,91,21);
    SWFDIR_REV   = new QRadioButton("Reverse",gbARB); SWFDIR_REV->setGeometry(150,166,91,21);
    // Add labels
    labels[0] = new QLabel("Frequency",gbARB);     labels[0]->setGeometry(10,26,80,16);
    labels[1] = new QLabel("Amplitude",gbARB);     labels[1]->setGeometry(10,48,80,16);
    labels[2] = new QLabel("Aux output",gbARB);    labels[2]->setGeometry(10,73,80,16);
    labels[3] = new QLabel("Offset output",gbARB); labels[3]->setGeometry(10,96,80,16);
    labels[4] = new QLabel("Waveform",gbARB);      labels[4]->setGeometry(10,118,80,16);
    labels[5] = new QLabel("Hz",gbARB);   labels[5]->setGeometry(200,26,31,21);
    labels[6] = new QLabel("Vp-p",gbARB); labels[6]->setGeometry(200,48,31,21);
    labels[7] = new QLabel("V",gbARB);    labels[7]->setGeometry(200,73,31,21);
    labels[8] = new QLabel("V",gbARB);    labels[8]->setGeometry(200,96,31,21);
    // Connect to the event slots
    leSWFREQ->setObjectName("leSWFREQ");
    leSWFVRNG->setObjectName("leSWFVRNG");
    leSWFVAUX->setObjectName("leSWFVAUX");
    leSWFVOFF->setObjectName("leSWFVOFF");
    SWFDIR_FWD->setObjectName("SWFDIR_FWD");
    SWFDIR_REV->setObjectName("SWFDIR_REV");
    foreach(QObject *w, gbARB->children())
    {
       if(w->objectName().startsWith("le")) connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(leChange()));
    }
    connect(Waveform,SIGNAL(currentIndexChanged(int)),this,SLOT(wfChange()));
    connect(EditWaveform,SIGNAL(pressed()),this,SLOT(wfEdit()));
    connect(SWFDIR_FWD,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
    connect(SWFDIR_REV,SIGNAL(clicked(bool)),this,SLOT(rbChange()));
}

bool ARBchannel::eventFilter(QObject *obj, QEvent *event)
{
    QLineEdit     *le;
    QString       res;
    float delta = 0;

   if (((obj == leSWFREQ) || (obj == leSWFVRNG) || (obj == leSWFVAUX) || (obj == leSWFVOFF)) && (event->type() == QEvent::KeyPress))
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
       if(delta != 0)
       {
          QString myString;
          if(obj == leSWFREQ) myString.sprintf("%1.0f", leSWFREQ->text().toFloat() + delta*100);
          if(obj == leSWFVRNG) myString.sprintf("%3.2f", leSWFVRNG->text().toFloat() + delta*10);
          if(obj == leSWFVAUX) myString.sprintf("%3.2f", leSWFVAUX->text().toFloat() + delta*10);
          if(obj == leSWFVOFF) myString.sprintf("%3.2f", leSWFVOFF->text().toFloat() + delta*10);
          ((QLineEdit *)obj)->setText(myString);
          ((QLineEdit *)obj)->setModified(true);
          ((QLineEdit *)obj)->editingFinished();
          UpdateOff = false;
          return true;
       }
   }
   UpdateOff = false;
   return QObject::eventFilter(obj, event);
}

QString ARBchannel::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + ",";
    res += leSWFREQ->text() + ",";
    if(isShutdown)
    {
        res += activeVRNG + ",";
        res += activeVAUX + ",";
        res += activeVOFF + ",";
    }
    else
    {
        res += leSWFVRNG->text() + ",";
        res += leSWFVAUX->text() + ",";
        res += leSWFVOFF->text() + ",";
    }
    if(SWFDIR_FWD->isChecked()) res += "FWD,";
    else res += "REV,";
    res += Waveform->currentText();
    return res;
}

bool ARBchannel::SetValues(QString strVals)
{
    QStringList resList;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!strVals.startsWith(title)) return false;
    resList = strVals.split(",");
    if(resList[0] != title) return false;
    if(resList.count() < 7) return false;
    leSWFREQ->setText(resList[1]);   leSWFREQ->setModified(true); leSWFREQ->editingFinished();
    if(isShutdown)
    {
        activeVRNG = resList[2];
        activeVAUX = resList[3];
        activeVOFF = resList[4];
    }
    else
    {
        leSWFVRNG->setText(resList[2]);  leSWFVRNG->setModified(true); leSWFVRNG->editingFinished();
        leSWFVAUX->setText(resList[3]);  leSWFVAUX->setModified(true); leSWFVAUX->editingFinished();
        leSWFVOFF->setText(resList[4]);  leSWFVOFF->setModified(true); leSWFVOFF->editingFinished();
    }
    if(resList[5] == "FWD")
    {
        SWFDIR_FWD->setChecked(true);
        SWFDIR_FWD->clicked(true);
    }
    else if(resList[5] == "REV")
    {
        SWFDIR_REV->setChecked(true);
        SWFDIR_REV->clicked(true);
    }
    int i = Waveform->findData(resList[6]);
    Waveform->setCurrentIndex(i);
    Waveform->currentIndexChanged(i);
    return true;
}

void ARBchannel::Update(void)
{
    QString res;

    if(comms==NULL) return;
    comms->rb.clear();
    if(UpdateOff) return;
    Updating = true;
    // Update the line edit boxes
    foreach(QObject *w, gbARB->children())
    {
       if(w->objectName().startsWith("le"))
       {
           res = comms->SendMess("G" + w->objectName().mid(3) + "," + QString::number(Channel) + "\n");
           if(!((QLineEdit *)w)->hasFocus()) if(res != "") ((QLineEdit *)w)->setText(res);
       }
    }
    // Update the waveform selection box
    res = comms->SendMess("GWFTYP," + QString::number(Channel) + "\n");
    if(res != "")
    {
       int i = Waveform->findData(res);
       Waveform->setCurrentIndex(i);
    }
    // Update the dirction radio buttons
    res = comms->SendMess("GWFDIR," + QString::number(Channel) +"\n");
    if(res == "FWD") SWFDIR_FWD->setChecked(true);
    if(res == "REV") SWFDIR_REV->setChecked(true);
    Updating = false;
}

void ARBchannel::leChange(void)
{
    QObject* obj = sender();
    QString res;

    if(comms == NULL) return;
    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2) +"," + QString::number(Channel);
    res += "," + ((QLineEdit *)obj)->text() + "\n";
    comms->SendCommand(res.toStdString().c_str());
    ((QLineEdit *)obj)->setModified(false);
}

void ARBchannel::rbChange(void)
{
    QString res = "SWFDIR,";
    if(comms == NULL) return;
    if(SWFDIR_FWD->isChecked()) res += QString::number(Channel) + ",FWD\n";
    if(SWFDIR_REV->isChecked()) res += QString::number(Channel) + ",REV\n";
    comms->SendCommand(res);
}

void ARBchannel::wfChange(void)
{
    if(comms == NULL) return;
    comms->SendCommand("SWFTYP," + QString::number(Channel) + "," + Waveform->currentText() + "\n");
}

void ARBchannel::ReadWaveform(void)
{
    int Wform[32];
    QString cmd,res;
    int i;

    // Read waveform
    ARBwfEdit->GetWaveform(Wform);
    // Send waveform to MIPS
    cmd = "SWFARB,"  + QString::number(Channel);

    for(i=0; i<PPP; i++) cmd += "," + QString::number(Wform[i]);
    cmd += "\n";
    if(comms == NULL) return;
    if(!comms->SendCommand(cmd))
    {
        if(statusBar != NULL) statusBar->showMessage("Error sending waveform to MIPS",2000);
    }
}

void ARBchannel::wfEdit(void)
{
    QString cmd,res;
    int Wform[32];
    int i;

    for(i=0; i<32; i++) Wform[i] = i*4 - 64;
    // Determine the PPP before we popup the edit dialog
    if(comms != NULL)
    {
        cmd =  "GARBPPP,"  + QString::number(Channel);
        res = comms->SendMess(cmd + "\n");
        PPP = res.toInt();
        if(PPP < 8) PPP = 8;
        if(PPP > 32) PPP = 32;
    }
    // Read the ARB waveform from MIPS
    if(comms != NULL) res = comms->SendMess("GWFARB," + QString::number(Channel) + "\n");
    if(res.contains("?"))
    {
        // Here if the message was NAKed
        if(statusBar != NULL) statusBar->showMessage("Error reading waveform from MIPS",2000);
        return;
    }
    QStringList Vals = res.split(",");
    for(i=0; i<PPP; i++)
    {
        if(i < Vals.count()) Wform[i] = Vals[i].toInt();
        else Wform[i] = 0;
    }
    ARBwfEdit = new ARBwaveformEdit(0,PPP);
    connect(ARBwfEdit, SIGNAL(WaveformReady()), this, SLOT(ReadWaveform()));
    ARBwfEdit->SetWaveform(Wform);
    ARBwfEdit->show();
}

void ARBchannel::Shutdown(void)
{
    if(isShutdown) return;
    isShutdown = true;
    activeVAUX = leSWFVAUX->text();
    leSWFVAUX->setText("0");
    leSWFVAUX->setModified(true);
    leSWFVAUX->editingFinished();
    activeVRNG = leSWFVRNG->text();
    leSWFVRNG->setText("0");
    leSWFVRNG->setModified(true);
    leSWFVRNG->editingFinished();
    activeVOFF = leSWFVOFF->text();
    leSWFVOFF->setText("0");
    leSWFVOFF->setModified(true);
    leSWFVOFF->editingFinished();
}

void ARBchannel::Restore(void)
{
    if(!isShutdown) return;
    isShutdown = false;
    leSWFVAUX->setText(activeVAUX);
    leSWFVAUX->setModified(true);
    leSWFVAUX->editingFinished();
    leSWFVRNG->setText(activeVRNG);
    leSWFVRNG->setModified(true);
    leSWFVRNG->editingFinished();
    leSWFVOFF->setText(activeVOFF);
    leSWFVOFF->setModified(true);
    leSWFVOFF->editingFinished();
}

QString ARBchannel::ProcessCommand(QString cmd)
{
    QLineEdit    *le = NULL;
    QRadioButton *rb = NULL;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    QStringList resList = cmd.split("=");
    if(resList[0].trimmed() == title + ".Frequency") le = leSWFREQ;
    else if(resList[0].trimmed() == title + ".Amplitude") le = leSWFVRNG;
    else if(resList[0].trimmed() == title + ".Aux output") le = leSWFVAUX;
    else if(resList[0].trimmed() == title + ".Offset output") le = leSWFVOFF;
    else if(resList[0].trimmed() == title + ".Forward") rb = SWFDIR_FWD;
    else if(resList[0].trimmed() == title + ".Reverse") rb = SWFDIR_REV;
    else if(resList[0].trimmed() == title + ".Waveform")
    {
       if(resList.count() == 1) return Waveform->currentText();
       int i = Waveform->findText(resList[1].trimmed());
       if(i<0) return "?";
       Waveform->setCurrentIndex(i);
       return "";
    }
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
        if(resList.count() == 1) if(rb->isChecked()) return "TRUE"; else return "FALSE";
        if(resList[1].trimmed() == "TRUE") rb->setChecked(true);
        if(resList[1].trimmed() == "FALSE") rb->setChecked(false);
        rb->clicked();
        return "";
    }
    return "?";
}
