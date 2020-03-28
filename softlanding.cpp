#include "softlanding.h"
#include "ui_softlanding.h"
#include <QDebug>
#include <QKeyEvent>

SoftLanding::SoftLanding(QWidget *parent, Comms *c, QStatusBar *statusbar) :
    QDialog(parent),
    ui(new Ui::SoftLanding)
{
    ui->setupUi(this);

    comms = c;
    sb = statusbar;
    Updating = false;
    UpdateOff = false;
    this->setFixedSize(1020,772);

    qApp->installEventFilter(this);  //in Dialog constructor
    QObjectList widgetList = ui->frmSL->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    widgetList += ui->gbSLRQ->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().contains("leS"))
        {
            ((QLineEdit *)w)->setValidator(new QDoubleValidator);
            connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(SLUpdated()));
        }
    }
    connect(ui->chkESIenable,SIGNAL(toggled(bool)),this,SLOT(ESIenable()));
    connect(ui->pbTuneF1,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbTuneF2,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbTuneCQ,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneF1,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneF2,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->pbRetuneCQ,SIGNAL(pressed()),this,SLOT(AutoTune()));
    connect(ui->rbRFDC,SIGNAL(clicked(bool)),this,SLOT(RQmode()));
    connect(ui->rbRFonly,SIGNAL(clicked(bool)),this,SLOT(RQmode()));
    connect(ui->pbShutdown,SIGNAL(pressed()),this,SLOT(Shutdown()));
    connect(ui->pbResetPowerSupply,SIGNAL(pressed()),this,SLOT(ResetPowerSupply()));
    connect(ui->MassUnits,SIGNAL(editingFinished()),this,SLOT(MassUnitsUpdated()));
}

void SoftLanding::reject()
{
    emit DialogClosed();
}

SoftLanding::~SoftLanding()
{
    delete ui;
}

bool SoftLanding::ConfigurationCheck(void)
{
    QString res;
    int     nRF,nDC,nESI,nDAC;
    QMessageBox msgBox;

    // If we are connected to MIPS test to make sure we have the following modules:
    //  2 - RF drivers
    //  2 - DC bias modules
    //  1 - ESI module
    //  1 - DAC module
    // Generate error message if not present
    if(comms->isConnected())
    {
       // Test for required modules
       res = comms->SendMessage("GCHAN,RF\n");
       nRF = res.toInt()/2;
       res = comms->SendMessage("GCHAN,DCB\n");
       nDC = res.toInt()/8;
       res = comms->SendMessage("GCHAN,ESI\n");
       nESI = res.toInt();
       res = comms->SendMessage("GCHAN,DAC\n");
       nDAC = res.toInt()/8;
       if((nRF < 2) || (nDC < 2) || (nESI < 1) || (nDAC < 1))
       {
           QString msg = "Invalid configuration!\n";
           msg += "   Required 2 RF modules, found " + QString::number(nRF) + "\n";
           msg += "   Required 2 DCbias modules, found " + QString::number(nDC) + "\n";
           msg += "   Required 1 ESI module, found " + QString::number(nESI) + "\n";
           msg += "   Required 1 DAC module, found " + QString::number(nDAC) + "\n";
           msgBox.setText(msg);
           msgBox.setInformativeText("Are you sure you want to continue?");
           msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
           msgBox.setDefaultButton(QMessageBox::No);
           int ret = msgBox.exec();
           if(ret == QMessageBox::No)
           {
               return false;
           }
       }
    }
    return true;
}

bool SoftLanding::eventFilter(QObject *obj, QEvent *event)
{
    QLineEdit *le;
    QString res;
    float delta = 0;

    if (((obj->objectName().startsWith("leS")) || (obj->objectName().startsWith("MassUnits"))) && (event->type() == QEvent::KeyPress))
    {
        if(Updating) return true;
        UpdateOff = true;
        le = (QLineEdit *)obj;
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
//        qDebug() << "pressed"<< key->key();
//        qDebug() << QApplication::queryKeyboardModifiers();
        if(key->key() == 16777235) delta = 0.1;
        if(key->key() == 16777237) delta = -0.1;
        if((QApplication::queryKeyboardModifiers() & 0x2000000) != 0) delta *= 10;
        if((QApplication::queryKeyboardModifiers() & 0x8000000) != 0) delta *= 100;
        if(delta != 0)
        {
           //le->setText(QString::number(le->text().toFloat() + delta));
           QString myString;
           if(obj->objectName().startsWith("leSRFFRQ"))
           {
               myString.sprintf("%3.0f", le->text().toFloat() + delta*10);
           }
           else if(obj->objectName().startsWith("leSDAC"))
           {
               myString.sprintf("%3.3f", le->text().toFloat() + delta/100);
           }
           else if(obj->objectName().startsWith("MassUnits"))
           {
               myString.sprintf("%3.0f", le->text().toFloat() + delta*10);
           }
           else
           {
               myString.sprintf("%3.2f", le->text().toFloat() + delta);
           }
           le->setText(myString);
           le->setModified(true);
           le->editingFinished();
           return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void SoftLanding::Save(QString Filename)
{
    QString res;

    if(Filename == "") return;
    QFile file(Filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        QDateTime dateTime = QDateTime::currentDateTime();
        stream << "# Soft Landing System settings, " + dateTime.toString() + "\n";
        QObjectList widgetList = ui->gbSLCQ->children();
        widgetList += ui->gbSLRQ->children();
        widgetList += ui->leSDCBOF_1;
        widgetList += ui->leSDCBOF_9;
        widgetList += ui->frmSL->children();
        widgetList += ui->gbSLfunnel1->children();
        widgetList += ui->gbSLfunnel2->children();
        foreach(QObject *w, widgetList)
        {
            if(w->objectName().mid(0,3) == "leS")
            {
                res = w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
                stream << res;
            }
        }
        stream << "MassUnits," + ui->MassUnits->text() + "\n";
        if(ui->chkESIenable->isChecked()) stream << "chkESIenable,TRUE\n";
        else stream << "chkESIenable,FALSE\n";
        if(ui->rbRFDC->isChecked()) stream << "rbRFDC,TRUE\n";
        else stream << "rbRFDC,FALSE\n";
        if(ui->rbRFonly->isChecked()) stream << "rbRFonly,TRUE\n";
        else stream << "rbRFonly,FALSE\n";
        file.close();
        sb->showMessage("Settings saved to " + Filename,2000);
    }
}

void SoftLanding::Load(QString Filename)
{
    QStringList resList;

    if(Filename == "") return;
    QObjectList widgetList = ui->frmSL->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    widgetList += ui->gbSLRQ->children();
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
                if(resList[0] == "MassUnits")
                {
                    ui->MassUnits->setText(resList[1]);
                    ui->MassUnits->setModified(true);
                    QMetaObject::invokeMethod(ui->MassUnits, "editingFinished");
                }
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
                }
                if(resList[0] == "chkESIenable")
                {
                    if(resList[1] == "TRUE") ui->chkESIenable->setChecked(true);
                    else ui->chkESIenable->setChecked(false);
                    QMetaObject::invokeMethod(ui->chkESIenable, "toggled");
                }
                if(resList[0] == "rbRFDC")
                {
                    if(resList[1] == "TRUE") ui->rbRFDC->setChecked(true);
                    else ui->rbRFDC->setChecked(false);
                    QMetaObject::invokeMethod(ui->rbRFDC, "clicked");
                }
                if(resList[0] == "rbRFonly")
                {
                    if(resList[1] == "TRUE") ui->rbRFonly->setChecked(true);
                    else ui->rbRFonly->setChecked(false);
                    QMetaObject::invokeMethod(ui->rbRFonly, "clicked");
                }
            }
        } while(!line.isNull());
        file.close();
        sb->showMessage("Settings loaded from " + Filename,2000);
    }
}

void SoftLanding::RQmode(void)
{
    if(ui->rbRFDC->isChecked()) comms->SendCommand("SDIO,A,1\n");
    else comms->SendCommand("SDIO,A,0\n");
}

void SoftLanding::ESIenable(void)
{
    if(ui->chkESIenable->isChecked()) comms->SendCommand("SHVENA,1\n");
    else  comms->SendCommand("SHVDIS,1\n");
}

void SoftLanding::AutoTune(void)
{
    QObject* obj = sender();
    int tch = 0, rtch = 0;
    bool stat=false;

    ((QPushButton *)(obj))->setDown(false);
    if(obj->objectName() == "pbTuneF1") tch = 1;
    else if(obj->objectName() == "pbTuneF2") tch = 2;
    else if(obj->objectName() == "pbTuneCQ") tch = 3;
    else if(obj->objectName() == "pbRetuneF1") rtch = 1;
    else if(obj->objectName() == "pbRetuneF2") rtch = 2;
    else if(obj->objectName() == "pbRetuneCQ") rtch = 3;
    QMessageBox msgBox;
    if(tch != 0)
    {
        QString msg = "This function will tune the RF head attached to channel " + QString::number(tch) + ". ";
        msg += "Make sure the RF head is attached and connected to your system as needed. ";
        msg += "This process can take up to 3 minutes.\n";
        msgBox.setText(msg);
        msgBox.setInformativeText("Are you sure you want to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    if(rtch != 0)
    {
        QString msg = "This function will retune the RF head attached to channel " + QString::number(tch) + ". ";
        msg += "Make sure the RF head is attached and connected to your system as needed. ";
        msg += "This process can take up to 1 minute.\n";
        msgBox.setText(msg);
        msgBox.setInformativeText("Are you sure you want to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) return;
    }
    if(tch == 1) stat = comms->SendCommand("TUNERFCH,1\n");
    else if(tch == 2) stat = comms->SendCommand("TUNERFCH,2\n");
    else if(tch == 3) stat = comms->SendCommand("TUNERFCH,3\n");
    else if(rtch == 1) stat = comms->SendCommand("RETUNERFCH,1\n");
    else if(rtch == 2) stat = comms->SendCommand("RETUNERFCH,2\n");
    else if(rtch == 3) stat = comms->SendCommand("RETUNERFCH,3\n");
    if(!stat)
    {
        QString msg = "Request failed!, could be a tune in process, only one channel ";
        msg += "can be tuned or retuned at a time. ";
        msgBox.setText(msg);
        msgBox.setInformativeText("");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}

// Set all output level to zero
void SoftLanding::Shutdown(void)
{
    QObjectList widgetList = ui->frmSL->children();
    widgetList += ui->leSRFDRV_1;
    widgetList += ui->leSRFDRV_2;
    widgetList += ui->leSRFDRV_3;
    widgetList += ui->gbSLRQ->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("leS"))
       {
           ((QLineEdit *)w)->setText("0");
           ((QLineEdit *)w)->setModified(true);
           QMetaObject::invokeMethod(w, "editingFinished");
       }
    }
}

void SoftLanding::ResetPowerSupply(void)
{
   comms->SendCommand("SDCPWR,ON\n");
}

// Here when the Mass units have changed, convert to volts
// and send to MIPS.
void SoftLanding::MassUnitsUpdated(void)
{
    QString myString;
    myString.sprintf("%5.4f\n", ui->MassUnits->text().toFloat() * 0.0025);
    comms->SendCommand("SDACV,CH8," + myString);
    UpdateOff = false;
}

void SoftLanding::SLUpdated(void)
{
    QObject* obj = sender();
    QString res;
    QMessageBox msgBox;
    static bool busy = false;

    if(Updating) return;
    if(!((QLineEdit *)obj)->isModified()) return;
    res = obj->objectName().mid(2).replace("_",",") + "," + ((QLineEdit *)obj)->text() + "\n";
    if(!comms->SendCommand(res.toStdString().c_str()))
    {
        if(!busy)
        {
           busy = true;
           QString msg = "Value rejected, likely out of range!";
           msgBox.setText(msg);
           msgBox.setInformativeText("");
           msgBox.setStandardButtons(QMessageBox::Ok);
           msgBox.exec();
        }
        busy = false;
        UpdateOff = false;
        ((QLineEdit *)obj)->setModified(false);
        return;
    }
    ((QLineEdit *)obj)->setModified(false);
    UpdateOff = false;
}

void SoftLanding::Update(void)
{
    QString res;

    if(UpdateOff) return;
    Updating = true;
    QObjectList widgetList = ui->frmSL->children();
    widgetList += ui->gbSLfunnel1->children();
    widgetList += ui->gbSLfunnel2->children();
    widgetList += ui->gbSLCQ->children();
    widgetList += ui->gbSLRQ->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().startsWith("le"))
       {
           if(!w->objectName().startsWith("leMass"))
           {
               if(!((QLineEdit *)w)->hasFocus())
               {
                  res = "G" + w->objectName().mid(3).replace("_",",") + "\n";
                  ((QLineEdit *)w)->setText(comms->SendMessage(res));
               }
           }
       }
    }
    // Read Mass units and convert to AMU, 2.5mV per AMU
    res = comms->SendMessage("GDACV,CH8\n");
    QString myString;
    myString.sprintf("%1.0f", res.toFloat()/0.0025);
    ui->MassUnits->setText(myString);
    // Read the RF out, ADC channel 0
    res = comms->SendMessage("ADC,0\n");
    myString.sprintf("%3.2f", res.toFloat()/409.6);
    ui->RFOUT->setText(myString);

    res = comms->SendMessage("GHVSTATUS,1\n");
    if(res == "ON") ui->chkESIenable->setChecked(true);
    else ui->chkESIenable->setChecked(false);
    res = comms->SendMessage("GDIO,A\n");
    if(res == "1") ui->rbRFDC->setChecked(1);
    else ui->rbRFonly->setChecked(1);
    ui->lblSLtime->setText(QDateTime::currentDateTime().toString());
    Updating = false;
}
