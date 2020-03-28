#include "pse.h"
#include "ui_pse.h"

#include <QDebug>
#include <QMessageBox>
#include <QFile>

namespace Ui {
class MIPS;
}

QT_USE_NAMESPACE

//static const char blankString[] = QT_TRANSLATE_NOOP("pseDialog", "N/A");

QDataStream &operator<<(QDataStream &out, const psgPoint &point)
{
    int i;

    out << point.Name << quint32(point.TimePoint);
    for(i=0;i<16;i++) out << point.DigitalO[i];
    for(i=0;i<16;i++) out << point.DCbias[i];
    out << point.Loop << point.LoopName << quint32(point.LoopCount);
    return out;
}

QDataStream &operator>>(QDataStream &in, psgPoint &point)
{
    QString Name;
    quint32 TimePoint;
    bool    DigitalO[16];
    float   DCbias[16];
    bool    Loop;
    QString LoopName;
    quint32 LoopCount;
    int     i;

    in >> Name;
    in >> TimePoint;
    for(i=0;i<16;i++) in >> DigitalO[i];
    for(i=0;i<16;i++) in >> DCbias[i];
    in >> Loop >> LoopName >> LoopCount;

    point.Name = Name;
    point.TimePoint = TimePoint;
    for(i=0;i<16;i++) point.DigitalO[i] = DigitalO[i];
    for(i=0;i<16;i++) point.DCbias[i] = DCbias[i];
    point.Loop = Loop;
    point.LoopName = LoopName;
    point.LoopCount = LoopCount;

    return in;
}

psgPoint::psgPoint()
{
    int  i;

    for(i=0;i<16;i++)
    {
        DigitalO[i] = false;
        DCbias[i] = 0.0;
    }
    Loop = false;
    LoopCount = 0;
    Name = "";
    LoopName = "";
    TimePoint = 0;
}

pseDialog::pseDialog(QList<psgPoint *> *psg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::pseDialog)
{
    ui->setupUi(this);
    // Make the dialog fixed size.
    this->setFixedSize(this->size());

    p = psg;
    activePoint = (*psg)[0];
    CurrentIndex = 0;
    UpdateDialog(activePoint);

//    pseDialog::setProperty("font", QFont("Times New Roman", 5));
    QObjectList widgetList = ui->gbDigitalOut->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("chkDO"))
       {
           connect(((QCheckBox *)w),SIGNAL(clicked(bool)),this,SLOT(on_DIO_checked()));
       }
    }
    ui->gbCurrentPoint->setTitle("Current time point: " + QString::number(CurrentIndex+1) + " of " + QString::number(p->size()));
    widgetList = ui->gbDCbias->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leDCB"))
       {
           connect(((QLineEdit *)w),SIGNAL(textEdited(QString)),this,SLOT(on_DCBIAS_edited()));
       }
    }
    connect(ui->pbInsertPulse, SIGNAL(pressed()), this, SLOT(InsertPulse()));
    connect(ui->pbInsertRamp, SIGNAL(pressed()), this, SLOT(MakeRamp()));
    connect(ui->pbInsertCancel, SIGNAL(pressed()), this, SLOT(RampCancel()));
    ui->gbInsertPulse->setVisible(false);
}

pseDialog::~pseDialog()
{
    delete ui;
}

void pseDialog::on_DCBIAS_edited()
{
    QObject* obj = sender();

    int Index = obj->objectName().mid(5).toInt() - 1;
    activePoint->DCbias[Index] = ((QLineEdit *)obj)->text().toFloat();
}

void pseDialog::on_DIO_checked()
{
    QObject* obj = sender();

    int Index = (int)obj->objectName().mid(5,1).toStdString().c_str()[0] - (int)'A';
    if(((QCheckBox *)obj)->isChecked()) activePoint->DigitalO[Index] = true;
    else  activePoint->DigitalO[Index] = false;
}

void pseDialog::UpdateDialog(psgPoint *point)
{
    QList<psgPoint*>::iterator it;
    int  Index;
    QString temp;

   temp = point->LoopName;
   ui->comboLoop->clear();
   ui->comboLoop->addItem("");
   for(it = p->begin(); it != p->end(); ++it) if(*it == point) break;
   else ui->comboLoop->addItem((*it)->Name);
   Index = ui->comboLoop->findText(temp);
   if( Index != -1) ui->comboLoop->setCurrentIndex(Index);
   ui->leName->setText(point->Name);
   ui->leClocks->setText(QString::number(point->TimePoint));
   ui->leCycles->setText(QString::number(point->LoopCount));
   if(point->Loop) ui->chkLoop->setChecked(true);
   else ui->chkLoop->setChecked(false);
   ui->comboLoop->setCurrentText(point->LoopName);
   QObjectList widgetList = ui->gbDigitalOut->children();
   foreach(QObject *w, widgetList)
   {
      if(w->objectName().contains("chkDO"))
      {
          Index = (int)w->objectName().mid(5,1).toStdString().c_str()[0] - (int)'A';
          ((QCheckBox *)w)->setChecked(point->DigitalO[Index]);
      }
   }
   widgetList = ui->gbDCbias->children();
   foreach(QObject *w, widgetList)
   {
      if(w->objectName().contains("leDCB"))
      {
          Index = w->objectName().mid(5).toInt() - 1;
          QString textV;
          textV.sprintf("%.2f", point->DCbias[Index]);
          ((QLineEdit *)w)->setText(textV);
      }
   }
}

void pseDialog::on_pbNext_pressed()
{
    if(CurrentIndex < p->size() - 1) CurrentIndex++;
    activePoint = (*p)[CurrentIndex];
    UpdateDialog(activePoint);
    ui->gbCurrentPoint->setTitle("Current time point: " + QString::number(CurrentIndex+1) + " of " + QString::number(p->size()));
}

void pseDialog::on_pbPrevious_pressed()
{
     if(CurrentIndex > 0) CurrentIndex--;
     activePoint = (*p)[CurrentIndex];
     UpdateDialog(activePoint);
     ui->gbCurrentPoint->setTitle("Current time point: " + QString::number(CurrentIndex+1) + " of " + QString::number(p->size()));
}

// Insert a new point after the current point
void pseDialog::on_pbInsert_pressed()
{
    psgPoint *point = new psgPoint;
    QList<psgPoint*>::iterator it;
    int i,deltaT;

    *point = *activePoint;
    // Generate the new time point's name. If the previous timepoint name ends with a
    // _number incement the number and see if the new name is unigue. If it is use it,
    // else append a _1 to the previous name.
    if((i = point->Name.lastIndexOf("_")) < 0)
    {
        point->Name += "_1";
    }
    else
    {
        // Here if _ was found so we assume a number follows
        point->Name = point->Name.mid(0,i+1) + QString::number(point->Name.mid(i+1).toInt()+1);
    }
    // See if this name is unique
    for(it = p->begin(); it != p->end(); ++it) if((*it)->Name == point->Name)
    {
        point->Name = activePoint->Name + "_1";
        break;
    }
    for(it = p->begin(); it != p->end(); ++it) if(*it == activePoint) break;
    int ctime = point->TimePoint;
    if(CurrentIndex >= p->size() - 1) deltaT = 100;
    else deltaT = ((*p)[CurrentIndex+1]->TimePoint - point->TimePoint)/2;
    it++;
    p->insert(it, point);
    CurrentIndex++;
    activePoint = point;
    point->TimePoint = ctime + deltaT;
    point->Loop = false;
    point->LoopCount = 0;
    point->LoopName = "";
    UpdateDialog(activePoint);
    ui->gbCurrentPoint->setTitle("Current time point: " + QString::number(CurrentIndex+1) + " of " + QString::number(p->size()));
}

// Delete the current point
void pseDialog::on_pbDelete_pressed()
{
    QList<psgPoint*>::iterator it,itnext;

    if(p->size() <= 1)
    {
        QMessageBox::information(NULL, "Error!", "Can't delete the only point in the sequence!");
        ui->pbDelete->setDown(false);
        return;
    }
    it = p->begin();
    for(it = p->begin(); it != p->end(); ++it) if(*it == activePoint) break;
    itnext =p->erase(it);
    activePoint = *itnext;
    CurrentIndex=0;
    for(it = p->begin(); it != p->end(); ++it,++CurrentIndex) if(*it == activePoint) break;
    if(CurrentIndex >= p->size()) CurrentIndex = p->size() - 1;
    activePoint = (*p)[CurrentIndex];
    UpdateDialog(activePoint);
    ui->gbCurrentPoint->setTitle("Current time point: " + QString::number(CurrentIndex+1) + " of " + QString::number(p->size()));
}

void pseDialog::on_leName_textChanged(const QString &arg1)
{
    activePoint->Name = arg1;
}

void pseDialog::on_leClocks_textChanged(const QString &arg1)
{
   activePoint->TimePoint = arg1.toInt();
}

void pseDialog::on_leCycles_textChanged(const QString &arg1)
{
    activePoint->LoopCount = arg1.toInt();
}

void pseDialog::on_chkLoop_clicked(bool checked)
{
    if(checked) activePoint->Loop = true;
    else activePoint->Loop = false;
}

void pseDialog::on_comboLoop_currentIndexChanged(const QString &arg1)
{
    activePoint->LoopName = arg1;
}

void pseDialog::InsertPulse(void)
{
    ui->gbInsertPulse->setVisible(true);
    ui->frmLoop->setVisible(false);
    ui->gbDigitalOut->setVisible(false);
    ui->gbDCbias->setVisible(false);
}

void pseDialog::MakeRamp(void)
{
    float restingV, pulseV;
    int   channel,width;

    channel = ui->lePulseChannel->text().toInt() - 1;
    if((channel<0)||(channel>15))
    {
        QMessageBox::information(NULL, "Error!", "Invalid channel number!");
        return;
    }
    // if the ramp up number of steps is 1 or less its a step function
    restingV = activePoint->DCbias[channel];
    pulseV = ui->lePulseVoltage->text().toFloat();
    width = ui->lePulseWidth->text().toInt();
    // Subtract half the rise time from the pulse width
    if(ui->leRampUp->text().toInt() > 1) width -= (ui->leRampStepSize->text().toInt() * ui->leRampUp->text().toInt())/2;
    //  Subtract half the fall time from the pulse width
    if(ui->leRampDwn->text().toInt() > 1) width -= (ui->leRampStepSize->text().toInt() * ui->leRampDwn->text().toInt())/2;
    if(width < 10)
    {
        QMessageBox::information(NULL, "Error!", "For the defined rise and fall times pulse width must be at least " + QString::number(ui->lePulseWidth->text().toInt()-width+10));
        return;
    }
    if(ui->leRampUp->text().toInt() <= 1)
    {
        activePoint->DCbias[channel] = pulseV;
        on_pbInsert_pressed();
        activePoint->TimePoint -= 100;
    }
    else
    {
        // Create the ramp
        for(int i=0; i<ui->leRampUp->text().toInt(); i++)
        {
            activePoint->DCbias[channel] += ((pulseV - restingV)/(ui->leRampUp->text().toInt()));
            activePoint->TimePoint += ui->leRampStepSize->text().toInt();
            UpdateDialog(activePoint);
            on_pbInsert_pressed();
            activePoint->TimePoint -= 100;
        }
    }
    // Now we are at the requested voltage so hold for the pulse width
    // time
    activePoint->TimePoint += width - 100;
    // Now ramp down
    if(ui->leRampDwn->text().toInt() <= 1)
    {
        activePoint->DCbias[ui->lePulseChannel->text().toInt()] = restingV;
        on_pbInsert_pressed();
    }
    else
    {
        // Create the ramp
        for(int i=0; i<ui->leRampDwn->text().toInt(); i++)
        {
            activePoint->DCbias[channel] -= ((pulseV - restingV)/(ui->leRampDwn->text().toInt()));
            activePoint->TimePoint += ui->leRampStepSize->text().toInt();
            UpdateDialog(activePoint);
            on_pbInsert_pressed();
            activePoint->TimePoint -= 100;
        }
        activePoint->TimePoint += 100;
    }
    activePoint->DCbias[channel] = restingV;
    UpdateDialog(activePoint);
    ui->gbInsertPulse->setVisible(false);
    ui->frmLoop->setVisible(true);
    ui->gbDigitalOut->setVisible(true);
    ui->gbDCbias->setVisible(true);
}

void pseDialog::RampCancel(void)
{
    UpdateDialog(activePoint);
    ui->gbInsertPulse->setVisible(false);
    ui->frmLoop->setVisible(true);
    ui->gbDigitalOut->setVisible(true);
    ui->gbDCbias->setVisible(true);
}
