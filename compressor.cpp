#include "compressor.h"
#include "ui_compressor.h"

Compressor::Compressor(QWidget *parent, QString name, QString MIPSname):
    QDialog(parent),
    ui(new Ui::Compressor)
{
    ui->setupUi(this);
    Updating = false;
    UpdateOff = false;
    comms = NULL;
    Title  = name;
    MIPSnm = MIPSname;

    QWidget::setWindowTitle(Title);
    QObjectList widgetList = ui->frmCompressor->children();
    widgetList += ui->gbARBmode->children();
    widgetList += ui->gbARBswitch->children();
    widgetList += ui->gbARBtiming->children();
    foreach(QObject *w, widgetList)
    {
       if(w->objectName().contains("leS")) connect(((QLineEdit *)w),SIGNAL(editingFinished()),this,SLOT(Updated()));
       else if(w->objectName().contains("chk")) connect(((QCheckBox *)w),SIGNAL(toggled(bool)),this,SLOT(Updated()));
       else if(w->objectName().contains("rb")) connect(((QRadioButton *)w),SIGNAL(clicked(bool)),this,SLOT(Updated()));
    }
    connect(ui->pbARBforceTrigger,SIGNAL(pressed()),this,SLOT(pbARBforceTriggerSlot()));
}

Compressor::~Compressor()
{
    delete ui;
}

void Compressor::Update(void)
{
    QStringList resList;
    QString res;
    QString CurrentList;
    int i;
    static bool inited = false;

    if(comms==NULL) return;
    if(UpdateOff) return;
    Updating = true;
    // Update all input boxes for the selected tab
    QObjectList widgetList = ui->frmCompressor->children();
    widgetList += ui->gbARBmode->children();
    widgetList += ui->gbARBswitch->children();
    widgetList += ui->gbARBtiming->children();
    foreach(QObject *w, widgetList)
    {
        comms->rb.clear();
        if(w->objectName().startsWith("leS") || w->objectName().startsWith("leG"))
        {
            if(!((QLineEdit *)w)->hasFocus())
            {
                res = "G" + w->objectName().mid(3) + "\n";
                ((QLineEdit *)w)->setText(comms->SendMess(res));
            }
        }
        else if(w->objectName().startsWith("chk"))
        {
            // Checkbox names encode the command and response for check and uncheck
            resList = w->objectName().split("_");
            if(resList.count() == 3)
            {
                res = "G" + resList[0].mid(3) + "\n";
                res = comms->SendMess(res);
                if(res == resList[1]) ((QCheckBox *)w)->setChecked(true);
                if(res == resList[2]) ((QCheckBox *)w)->setChecked(false);
                if(res.contains("?")) comms->waitforline(100);
            }
        }
        else if(w->objectName().startsWith("rb"))
        {
            resList = w->objectName().split("_");
            if(resList.count() == 2)
            {
                res = "G" + resList[0].mid(3) + "\n";
                res = comms->SendMess(res);
                if(res == resList[1]) ((QRadioButton *)w)->setChecked(true);
            }
        }
        else if(w->objectName().startsWith("lel"))
        {
            // This is a list of parameters from a command common to several line edit boxes
            resList = w->objectName().split("_");
            if(resList.count() == 2)
            {
                i = resList[1].toInt();
                res = resList[0].mid(3);
                if(!CurrentList.startsWith(res)) CurrentList = res + "," + comms->SendMess(res + "\n");
                resList = CurrentList.split(",");
                if(resList.count() > i) ((QLineEdit *)w)->setText(resList[i]);
            }
        }
    }
    Updating = false;
    inited = true;
}

void Compressor::Updated(void)
{
    QObject*    obj = sender();
    QString     res;
    QStringList resList;

    if(comms==NULL) return;
    if(Updating) return;
    if(obj->objectName().startsWith("leS"))
    {
        if(!((QLineEdit *)obj)->isModified()) return;
        res = obj->objectName().mid(2) + "," + ((QLineEdit *)obj)->text() + "\n";
        comms->SendCommand(res.toStdString().c_str());
        ((QLineEdit *)obj)->setModified(false);
    }
    if(obj->objectName().startsWith("chkS"))
    {
        resList = obj->objectName().mid(3).split("_");
        if(resList.count() == 3)
        {
            if(((QCheckBox *)obj)->isChecked()) comms->SendCommand(resList[0] + "," + resList[1] + "\n");
            else comms->SendCommand(resList[0] + "," + resList[2] + "\n");
        }
    }
    if(obj->objectName().startsWith("rbS"))
    {
        resList = obj->objectName().mid(2).split("_");
        if(resList.count() == 2)
        {
            if(((QRadioButton *)obj)->isChecked()) comms->SendCommand(resList[0] + "," + resList[1] + "\n");
        }
    }
}

QString Compressor::Report(void)
{
   QString res;
   QStringList resList;

   res.clear();
   QObjectList widgetList = ui->frmCompressor->children();
   widgetList += ui->gbARBmode->children();
   widgetList += ui->gbARBswitch->children();
   widgetList += ui->gbARBtiming->children();
   foreach(QObject *w, widgetList)
   {
       if(w->objectName().startsWith("leS"))
       {
           res += Title + "," + w->objectName() + "," + ((QLineEdit *)w)->text() + "\n";
       }
       if(w->objectName().startsWith("chkS"))
       {
           resList = w->objectName().split("_");
           if(resList.count() == 3)
           {
               if(((QCheckBox *)w)->isChecked()) res += Title + "," + resList[0] + "," + resList[1] + "\n";
               else res += Title + "," + resList[0] + "," + resList[2] + "\n";
           }
       }
       if(w->objectName().startsWith("rbS"))
       {
           resList = w->objectName().split("_");
           if(resList.count() == 2) if(((QRadioButton *)w)->isChecked()) res += Title + "," + resList[0] + "," + resList[1] + "\n";
       }
   }
   return res;
}

bool Compressor::SetValues(QString strVals)
{
    QStringList resList,ctrlList;

    if(!strVals.startsWith(Title)) return false;
    resList = strVals.split(",");
    if(resList[0] != Title) return false;
    if(resList.count() < 3) return false;
    QObjectList widgetList = ui->frmCompressor->children();
    widgetList += ui->gbARBmode->children();
    widgetList += ui->gbARBswitch->children();
    widgetList += ui->gbARBtiming->children();
    foreach(QObject *w, widgetList)
    {
        if(w->objectName().startsWith(resList[1]))
        {
            if(w->objectName().startsWith("leS"))
            {
                ((QLineEdit *)w)->setText(resList[2]);
                ((QLineEdit *)w)->setModified(true);
                ((QLineEdit *)w)->editingFinished();
                return true;
            }
            if(w->objectName().startsWith("chk"))
            {
                ctrlList = w->objectName().split("_");
                if(ctrlList.count() >= 3)
                {
                    if((ctrlList[0] == resList[1]) && (ctrlList[1] == resList[2])) {((QCheckBox *)w)->setChecked(true); return(true);}
                    if((ctrlList[0] == resList[1]) && (ctrlList[2] == resList[2])) {((QCheckBox *)w)->setChecked(true); return(true);}
                    ((QCheckBox *)w)->clicked();
                }
            }
            if(w->objectName().startsWith("rb"))
            {
                if(w->objectName() == (resList[1] + "_" + resList[2]))
                {
                    ((QRadioButton *)w)->setChecked(true);
                    ((QRadioButton *)w)->clicked();
                    return true;
                }
            }
        }
    }
    return false;
}

void Compressor::pbARBforceTriggerSlot(void)
{
    if(comms==NULL) return;
    comms->SendCommand("TARBTRG\n");
}

QString Compressor::ProcessCommand(QString cmd)
{
    QLineEdit    *le    = NULL;
    QRadioButton *rb    = NULL;
    QComboBox    *combo = NULL;
    QPushButton  *pb    = NULL;

    if(!cmd.startsWith(Title)) return "?";
    QStringList resList = cmd.split("=");
    if(resList[0].trimmed() == Title + ".Order") le = ui->leSARBCORDER;
    else if(resList[0].trimmed() == Title + ".Table") le = ui->leSARBCTBL;
    else if(resList[0].trimmed() == Title + ".Compress time") le = ui->leSARBCTC;
    else if(resList[0].trimmed() == Title + ".Trigger delay") le = ui->leSARBCTD;
    else if(resList[0].trimmed() == Title + ".Normal time") le = ui->leSARBCTN;
    else if(resList[0].trimmed() == Title + ".Non compress time") le = ui->leSARBCTNC;
    else if(resList[0].trimmed() == Title + ".Compress") rb = ui->rbSARBCMODE_Compress;
    else if(resList[0].trimmed() == Title + ".Normal") rb = ui->rbSARBCMODE_Normal;
    else if(resList[0].trimmed() == Title + ".Close") rb = ui->rbSARBCSW_Close;
    else if(resList[0].trimmed() == Title + ".Open") rb = ui->rbSARBCSW_Open;
    else if(resList[0].trimmed() == Title + ".Trigger") pb = ui->pbARBforceTrigger;
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
