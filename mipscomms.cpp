#include "mipscomms.h"
#include "ui_mipscomms.h"

MIPScomms::MIPScomms(QWidget *parent, QList<Comms*> S) :
    QDialog(parent),
    ui(new Ui::MIPScomms)
{
    ui->setupUi(this);

    Systems = S;

    // Fill selection boxes with MIPS names
    ui->comboTermMIPS->clear();
    for(int i=0;i<Systems.count();i++)
    {
        ui->comboTermMIPS->addItem(Systems[i]->MIPSname);
    }
    connect(ui->comboTermCMD->lineEdit(),SIGNAL(editingFinished()),this,SLOT(CommandEntered()));
    if(Systems.count() > 0)
    {
        for(int i=0;i<Systems.count();i++) connect(Systems.at(i), SIGNAL(DataReady()),this, SLOT(readData2Console()));
    }
}

MIPScomms::~MIPScomms()
{
    if(Systems.count() > 0)
    {
        // disconnect all ports from terminal read routine
        for(int i=0;i<Systems.count();i++) disconnect(Systems.at(i), SIGNAL(DataReady()),0,0);
    }
    delete ui;
}

void MIPScomms::reject()
{
    emit DialogClosed();
    delete this;
}

void MIPScomms::CommandEntered(void)
{
    QString res;

    res = ui->comboTermCMD->currentText() + "\n";
    if(Systems.count() > 0)
    {
        for(int i=0;i<Systems.count();i++)
        {
            Systems.at(i)->SendString(ui->comboTermMIPS->currentText(),res);
        }
    }
    ui->comboTermCMD->setCurrentText("");
}

void MIPScomms::readData2Console(void)
{
    QByteArray data;

    if(Systems.count() > 0)
    {
         for(int i=0;i<Systems.count();i++)
         {
             data = Systems.at(i)->readall();
             ui->txtTerm->insertPlainText(QString(data));
             ui->txtTerm->ensureCursorVisible();
         }
    }
}

