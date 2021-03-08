#include "device.h"
#include <stdio.h>
#include <stdlib.h>

Device::Device(QWidget *parent, QString Name, QString FileName,QString units,  int x, int y)
{
   InitList.clear();
   p          = parent;
   Title      = Name;
   X          = x;
   Y          = y;
   ConfigFile = FileName;
   M          = 1;
   B          = 0;
   Units      = units;
   serial = new QSerialPort(this);
}

Device::~Device()
{
    if(serial->isOpen()) serial->close();
}

void Device::Show(void)
{
    // Display UI
    frmDevice = new QFrame(p);               frmDevice->setGeometry(X,Y,180,21);
    leDevice  = new QLineEdit(frmDevice);    leDevice->setGeometry(70,0,70,21);
    labels[0] = new QLabel(Title,frmDevice); labels[0]->setGeometry(0,0,59,16);
    labels[1] = new QLabel(Units,frmDevice); labels[1]->setGeometry(150,0,31,16);
    // Open the device file and load all the parameter
    LoadDeviceConfig();
    // Open the comm port and initalize the device
    serial->setPortName(Port);
    if (serial->open(QIODevice::ReadWrite))
    {
        //Here if open with no error, send the init strings
        for(int i=0;i<InitList.count();i++)
        {
            serial->write(InitList[i].toStdString().c_str());
            serial->write(EOL.toStdString().c_str());
        }
        serial->close();
    }
    else
    {
        // Here with open error, write error message to text box
        leDevice->setText("Port: " + Port + " failed to open!");
    }
}

void Device::LoadDeviceConfig(void)
{
    QStringList resList;

    QFile file(ConfigFile);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            if(line.trimmed().startsWith("#")) continue;
            resList = line.split(",");
            if((resList[0].toUpper() == "PORT") && (resList.length()==2)) Port = resList[1];
            else if((resList[0].toUpper() == "SETTING") && (resList.length()==5))
            {
                if(resList[1] == "9600") serial->setBaudRate(QSerialPort::Baud9600);
                else if(resList[1] == "19200") serial->setBaudRate(QSerialPort::Baud19200);
                else if(resList[1] == "38400") serial->setBaudRate(QSerialPort::Baud38400);
                else if(resList[1] == "57600") serial->setBaudRate(QSerialPort::Baud57600);
                else if(resList[1] == "115200") serial->setBaudRate(QSerialPort::Baud115200);

                if(resList[2] == "5") serial->setDataBits(QSerialPort::Data5);
                else if(resList[2] == "6") serial->setDataBits(QSerialPort::Data6);
                else if(resList[2] == "7") serial->setDataBits(QSerialPort::Data7);
                else if(resList[2] == "8") serial->setDataBits(QSerialPort::Data8);

                if(resList[3].toUpper() == "N") serial->setParity(QSerialPort::NoParity);
                else if(resList[3].toUpper() == "O") serial->setParity(QSerialPort::OddParity);
                else if(resList[3].toUpper() == "E") serial->setParity(QSerialPort::EvenParity);
                else if(resList[3].toUpper() == "M") serial->setParity(QSerialPort::MarkParity);
                else if(resList[3].toUpper() == "S") serial->setParity(QSerialPort::SpaceParity);

                if(resList[4] == "1") serial->setStopBits(QSerialPort::OneStop);
            #ifdef Q_OS_WIN
                else if(resList[4] == "1.5") serial->setStopBits(QSerialPort::OneAndHalfStop);
            #endif
                else if(resList[4] == "2") serial->setStopBits(QSerialPort::TwoStop);
                serial->setFlowControl(QSerialPort::NoFlowControl);
            }
            else if(resList[0].toUpper() == "EOL")
            {
                EOL.clear();
                for(int i=1;i<resList.count();i++)
                {
                    if(resList[i] == "CR") EOL += "\r";
                    if(resList[i] == "LF") EOL += "\n";
                }
            }
            else if((resList[0].toUpper() == "INIT") && (resList.length()==1))
            {
                do
                {
                    line = stream.readLine();
                    if(line.trimmed().startsWith("#")) continue;
                    if(line.toUpper() == "END INIT") break;
                    InitList.append(line);
                } while(!line.isNull());
            }
            else if((resList[0].toUpper() == "READ COMMAND") && (resList.length()==2)) ReadCommand = resList[1];
            else if((resList[0].toUpper() == "SCAN FORMAT") && (resList.length()==2)) ScanFormat = resList[1];
            else if((resList[0].toUpper() == "M") && (resList.length()==2)) M = resList[1].toFloat();
            else if((resList[0].toUpper() == "B") && (resList.length()==2)) B = resList[1].toFloat();
        } while(!line.isNull());
    }
    file.close();
}

// Called at the system's update rate.
// - If the port is closed, open it and send command to read data.
// - If port is open, read the data then close the port.
// This logic will allow the port to be shared when one device
// has multiple signal sources.
void Device::Update(void)
{
   float fval=0;

   if(serial->isOpen())
   {
       // Look for data and read.
       QByteArray data = serial->readAll();
       QString str = QString::fromStdString(data.toStdString());
       std::sscanf(str.toStdString().c_str(), ScanFormat.toStdString().c_str(),&fval);
       leDevice->setText(QString::number(fval * M + B));
       serial->close();
   }
   else
   {
       if (serial->open(QIODevice::ReadWrite))
       {
           // Send read command, we will read the data on the next call
           serial->write(ReadCommand.toStdString().c_str());
           serial->write(EOL.toStdString().c_str());
       }
   }
}

QString Device::Report(void)
{
    QString res;
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    res = title + "," + leDevice->text();
    return(res);
}

QString Device::ProcessCommand(QString cmd)
{
    QString title;

    title.clear();
    if(p->objectName() != "") title = p->objectName() + ".";
    title += Title;
    if(!cmd.startsWith(title)) return "?";
    if(cmd == title)
    {
        return leDevice->text();
    }
    if(cmd == title + ".Update")
    {
        Update();
        return "";
    }
    return "?";
}
