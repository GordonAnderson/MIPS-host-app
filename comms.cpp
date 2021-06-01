#include <QTime>
#include "comms.h"

Comms::Comms(SettingsDialog *settings, QString Host, QStatusBar *statusbar)
{
    p = settings->settings();
    sb = statusbar;
    client_connected = false;
    host = Host;
    properties = NULL;
    serial = new QSerialPort(this);
    keepAliveTimer = new QTimer;
    reconnectTimer = new QTimer;
    connect(&client, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    connect(&client, SIGNAL(connected()),this, SLOT(connected()));
    connect(&client, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(&client, SIGNAL(aboutToClose()),this, SLOT(slotAboutToClose()));
    connect(keepAliveTimer, SIGNAL(timeout()), this, SLOT(slotKeepAlive()));
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(slotReconnect()));
}

char Comms::getchar(void)
{
    return rb.getch();
}

int Comms::CalculateCRC(QByteArray fdata)
{
    unsigned char generator = 0x1D;
    unsigned char crc = 0;

    for(int i=0; i<fdata.count(); i++)
    {
      crc ^= fdata[i];
      for(int j=0; j<8; j++)
      {
        if((crc & 0x80) != 0)
        {
          crc = ((crc << 1) ^ generator);
        }
        else
        {
          crc <<= 1;
        }
      }
    }
    return crc;
}

// Reads an ADC vector from MIPS. This function will setup for data from the
// serial port and then fill the buffer passed by reference if valid. This
// function does not block. After a vector is received a signal is sent and
// then the system is reset for the next vector. After all vectors are read
// the comms are reset to normal command processing
void Comms::GetADCbuffer(quint16 *ADCbuffer, int NumSamples)
{
    if(ADCbuffer == NULL) return;
    // Save the pointer and maximum length parameters
    ADCbuf = ADCbuffer;
    ADClen = NumSamples;
    ADCstate = WaitingForHeader;
    // Redirect the input data processing to ADC buffer processing
    disconnect(&client, SIGNAL(readyRead()), 0, 0);
    disconnect(serial, SIGNAL(readyRead()), 0, 0);
    connect(&client, SIGNAL(readyRead()), this, SLOT(readData2ADCBuffer()));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData2ADCBuffer()));
}

// Release the ADC buffer collection mode and return the processing routine
// for incoming data to the ringbuffers
void Comms::ADCrelease(void)
{
    // Connect the data read signal back to the ring buffers
    disconnect(&client, SIGNAL(readyRead()), 0, 0);
    disconnect(serial, SIGNAL(readyRead()), 0, 0);
    connect(&client, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData2RingBuffer()));
    ADCbuf = NULL;
}

void Comms::readData2ADCBuffer(void)
{
    int i;
    QByteArray data;
    static quint8 last = 0, *b;
    static int DataPtr;
    static int Vlength=0,Vnum=0;
    static bool Vlast;

    //qDebug() << "read data to adc buffer " << ADCstate;
    if(ADCbuf == NULL) return;
    if(client.isOpen()) data = client.readAll();
    if(serial->isOpen()) data = serial->readAll();
    for(i=0;i<data.size();i++)
    {
        switch(ADCstate)
        {
           case WaitingForHeader:
            // look for 0x55, 0xAA. This flags start of data
            if((last == 0x55) && ((quint8)data[i] == 0xAA))
            {
                ADCstate = ReadingHeader;
                DataPtr =0;
                b = (quint8 *)&Vlength;
                continue;
            }
            break;
          case ReadingHeader:
            //qDebug() << "reading header";
            // Read, 24 bit length, 16 bit vector num, 8 bit last vector flag 0xFF if last
            if(DataPtr == 0) b[0] = data[i];
            if(DataPtr == 1) b[1] = data[i];
            if(DataPtr == 2) { b[2] = data[i]; b = (quint8 *)&Vnum; }
            if(DataPtr == 3) b[0] = data[i];
            if(DataPtr == 4) b[1] = data[i];
            if(DataPtr == 5)
            {
                if((unsigned char)data[i] == 0xFF) Vlast =  true;
                else Vlast = false;
            }
            DataPtr++;
            if(DataPtr > 5)
            {
                DataPtr = 0;
                ADCstate = ReadingData;
                b = (quint8 *)ADCbuf;
                continue;
            }
            break;
          case ReadingData:
            //qDebug() << "reading data " << DataPtr;
            // Read the data block
            b[DataPtr++] = data[i];
            if(DataPtr >= (Vlength * 2))
            {
                ADCstate = ReadingTrailer;
            }
            break;
          case ReadingTrailer:
            //qDebug() << "reading trailer";
            // look for 0xAE, 0xEA. This flags end of message
            if((last == 0xAE) && ((quint8)data[i] == 0xEA))
            {
                emit ADCvectorReady();
                if(Vlast)
                {
                    ADCstate = ADCdone;
                }
                else ADCstate = WaitingForHeader;
                if(ADCstate == WaitingForHeader) continue;
            }
            if(ADCstate != ADCdone) break;
          case ADCdone:
            //qDebug() << "reading done";
            // ADC done so send signal
            emit ADCrecordingDone();
            return;
          default:
            last = data[i];
            break;
        }
        last = data[i];
    }
}


void Comms::GetMIPSfile(QString MIPSfile, QString LocalFile)
{
    bool ok = false;
    QString FileData;
    char c;
    int len;

    // Send the command to MIPS to get the file
    if(SendCommand("GET," + MIPSfile + "\n"))
    {
       // Now process the data in the ring buffer
       // First read the length
        waitforline(500);
        QString FileSize = getline();
        // Read the hex data stream
        for(int i=0; i<FileSize.toInt(); i+=1024)
        {
            len = 1024;
            if((FileSize.toInt() - i) < 1024) len = FileSize.toInt() - i;
            // Read the block of data
            for(int j=0; j<len*2; j++)
            {
              while((c = getchar()) == 0) QApplication::processEvents();
              if(c == '\n') break;
              FileData += QChar(c);
            }
            // Send "Next" block request
            if((FileSize.toInt()-i) > 1024) SendString("Next\n");
        }
        while((c = getchar()) == 0) QApplication::processEvents();
        if(FileData.count() != 2 * FileSize.toInt())
        {
            // Here if file data block is not the proper size
            QMessageBox::critical(this, tr("File Error"), "Data block size not correct!");
            return;
        }
        // Read the CRC
        waitforline(500);
        QString FileCRC = getline();
        // Convert data to byte array and callculate CRC
        QByteArray fdata;
        fdata.resize(FileSize.toInt());
        for(int i=0; i<FileSize.toInt(); i++)
        {
            fdata[i] = FileData.mid(i*2,2).toUInt(&ok,16);
        }
        if(CalculateCRC(fdata) == FileCRC.toInt())
        {
            // Now save the data to the user selected file
            QFile file(LocalFile);
            file.open(QIODevice::WriteOnly);
            file.write(fdata);
            file.close();
            QMessageBox::information(this, tr("File saved"), "File from MIPS read and saved successfully!");
        }
        else
        {
            // Here with file CRC error, raise error message
            QMessageBox::critical(this, tr("File Error"), "CRC error!");
        }
    }
}

void Comms::PutMIPSfile(QString MIPSfile, QString LocalFile)
{
    QByteArray fdata;
    int fsize,len;
    QString dblock,res;

    // open the local file to send and read the data
    QFile file(LocalFile);
    file.open(QIODevice::ReadOnly);
    fdata = file.readAll();
    // Convert the data into a ascii hex string
    for(int i=0; i<file.size(); i++)
    {
         dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
    }
    fsize = file.size();
    file.close();
    // Send the data to MIPS
    if(SendCommand("PUT," + MIPSfile + "," + QString().number(fsize) + "\n"))
    {
        for(int i=0; i<dblock.count(); i+=1024)
        {
            len = 1024;
            if((dblock.count() - i) < 1024) len = dblock.count() - i;
            // Send this in chunks in case the sender is a lot faster than MIPS
            for(int k=0;k<len;k+=128)
            {
                if(len > (k + 128)) SendString(dblock.mid(i+k,128));
                else SendString(dblock.mid(i+k,len - k));
                msDelay(5);
            }
             //SendString(dblock.mid(i,len));
            if((len == 1024) && ((dblock.count() - i) != 1024))
            {
                waitforline(1000);
                if((res = getline()) == "")
                {
                    // Here if we timed out, display error and exit
                    QMessageBox::critical(this, tr("Data read error"), "Timedout waiting for data from MIPS!");
                    return;
                }
            }
            QApplication::processEvents();
        }
        SendString("\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("File saved"), "File sent to MIPS!");
    }
}

void Comms::GetEEPROM(QString FileName, QString Board, int Addr)
{
    bool ok = false;
    QString FileData;

    // Send the command to MIPS to get the file
    if(SendCommand("GETEEPROM," + Board + "," + QString().sprintf(",%x\n",Addr)))
    {
       // Now process the data in the ring buffer
       // First read the length
        waitforline(500);
        QString FileSize = getline();
        // Read the hex data stream
        waitforline(1000);
        FileData = getline();
        if(FileData.count() != 2 * FileSize.toInt())
        {
            // Here if file data block is not the proper size
            QMessageBox::critical(this, tr("File Error"), "Data block size not correct!");
            return;
        }
        // Read the CRC
        waitforline(500);
        QString FileCRC = getline();
        // Convert data to byte array and callculate CRC
        QByteArray fdata;
        fdata.resize(FileSize.toInt());
        for(int i=0; i<FileSize.toInt(); i++)
        {
            fdata[i] = FileData.mid(i*2,2).toUInt(&ok,16);
        }
        if(CalculateCRC(fdata) == FileCRC.toInt())
        {
            // Now save the data to the user selected file
            QFile file(FileName);
            file.open(QIODevice::WriteOnly);
            file.write(fdata);
            file.close();
            QMessageBox::information(this, tr("File saved"), "EEPROM from MIPS read and saved successfully!");
        }
        else
        {
            // Here with file CRC error, raise error message
            QMessageBox::critical(this, tr("File Error"), "CRC error!");
        }
    }
}

void Comms::GetARBFLASH(QString FileName)
{
    bool ok = false;
    QString FileData;

    if(SendCommand("GETFLASH\n"))
    {
       // Now process the data in the ring buffer
       // First read the length
        waitforline(500);
        QString FileSize = getline();
        // Read the hex data stream
        waitforline(1000);
        FileData = getline();
        if(FileData.count() != 2 * FileSize.toInt())
        {
            // Here if file data block is not the proper size
            QMessageBox::critical(this, tr("File Error"), "Data block size not correct!");
            return;
        }
        // Read the CRC
        waitforline(500);
        QString FileCRC = getline();
        // Convert data to byte array and callculate CRC
        QByteArray fdata;
        fdata.resize(FileSize.toInt());
        for(int i=0; i<FileSize.toInt(); i++)
        {
            fdata[i] = FileData.mid(i*2,2).toUInt(&ok,16);
        }
        if(CalculateCRC(fdata) == FileCRC.toInt())
        {
            // Now save the data to the user selected file
            QFile file(FileName);
            file.open(QIODevice::WriteOnly);
            file.write(fdata);
            file.close();
            QMessageBox::information(this, tr("File saved"), "FLASH from ARB read and saved successfully!");
        }
        else
        {
            // Here with file CRC error, raise error message
            QMessageBox::critical(this, tr("File Error"), "CRC error!");
        }
    }
}

void Comms::PutARBFLASH(QString FileName)
{
    QString FileData;
    QString dblock;
    QByteArray fdata;

    if(SendCommand("PUTFLASH\n"))
    {
        // open the local file to send and read the data
        QFile file(FileName);
        file.open(QIODevice::ReadOnly);
        fdata = file.readAll();
        // Convert the data into a ascii hex string
        for(int i=0; i<file.size(); i++)
        {
             dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
        }
        SendString(QString().number(file.size()) + "\n");
        file.close();
        // Send the data to ARB
        SendString(dblock + "\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("FLASH write"), "ARB module FLASH Written!");
    }
}

void Comms::PutEEPROM(QString FileName, QString Board, int Addr)
{
    QString FileData;
    QString dblock;
    QByteArray fdata;

    // Send the command to MIPS to get the file
    if(SendCommand("PUTEEPROM," + Board + QString().sprintf(",%x\n",Addr)))
    {
        // open the local file to send and read the data
        QFile file(FileName);
        file.open(QIODevice::ReadOnly);
        fdata = file.readAll();
        // Convert the data into a ascii hex string
        for(int i=0; i<file.size(); i++)
        {
             dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
        }
        //SendCommand("\n");
        SendString(QString().number(file.size()) + "\n");
        file.close();
        // Send the data to MIPS
        SendString(dblock + "\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("EEPROM write"), "MIPS module's EEPROM Written!");
    }
}

void Comms::ARBupload(QString Faddress, QString FileName)
{
    QByteArray fdata;
    int fsize,len;
    QString dblock,res;

    // open the local file to send and read the data
    QFile file(FileName);
    file.open(QIODevice::ReadOnly);
    fdata = file.readAll();
    // Convert the data into a ascii hex string
    for(int i=0; i<file.size(); i++)
    {
         dblock += QString().sprintf("%02x",(unsigned char)fdata[i]);
    }
    fsize = file.size();
    file.close();
    // Send the data to ARB
    if(SendCommand("ARBPGM," + Faddress + "," + QString().number(fsize) + "\n" ))
    {
        for(int i=0; i<dblock.count(); i+=512)
        {
            len = 512;
            if((dblock.count() - i) < 512) len = dblock.count() - i;
            // Send this in chunks in case the sender is a lot faster than MIPS
            for(int k=0;k<len;k+=128)
            {
                if(len > (k + 128)) SendString(dblock.mid(i+k,128));
                else SendString(dblock.mid(i+k,len - k));
                msDelay(10);
            }
//            SendString(dblock.mid(i,len));
            if(len == 512)
            {
                waitforline(2000);
                if((res = getline()) == "")
                {
                    // Here if we timed out, display error and exit
                    QMessageBox::critical(this, tr("Data read error"), "Timedout waiting for data from ARB!");
                    return;
                }
            }
            QApplication::processEvents();
        }
        SendString("\n");
        SendString(QString().number(CalculateCRC(fdata)) + "\n");
        QMessageBox::information(this, tr("File saved"), "File uploaded to ARB FLASH!");
    }
}

bool Comms::SendString(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return SendString(message);
    return false;
}

bool Comms::SendString(QString message)
{
    if (!serial->isOpen() && !client.isOpen())
    {
        if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + " SS Disconnected!",2000);
        else sb->showMessage("SS Disconnected!",2000);
        return true;
    }
    if(client.isOpen()) keepAliveTimer->setInterval(600000);
    if (serial->isOpen())
    {
        QString m;
        if(message.length() > 100) for(int i=0;i<message.length();i++)
        {
            m = message.at(i);
            serial->write(m.toStdString().c_str());
            if(((i+1) % 100) == 0) msDelay(10);
        }
        else serial->write(message.toStdString().c_str());
    }
    if (client.isOpen()) client.write(message.toStdString().c_str());
    return true;
}

bool Comms::SendCommand(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return(SendCommand(message));
    return false;
}

bool Comms::SendCommand(QString message)
{
    QString res;

    if (!serial->isOpen() && !client.isOpen())
    {
        if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + " SC Disconnected!",2000);
        else sb->showMessage("SC Disconnected!",2000);
        return true;
    }
    if(client.isOpen()) keepAliveTimer->setInterval(600000);
    for(int i=0;i<2;i++)
    {
        rb.clear();
        if (serial->isOpen())
        {
            QString m;
            if(message.length() > 100) for(int j=0;j<message.length();j++)
            {
                m = message.at(j);
                serial->write(m.toStdString().c_str());
                if(((j+1) % 100) == 0) msDelay(10);
            }
            else serial->write(message.toStdString().c_str());
        }
        if (client.isOpen()) client.write(message.toStdString().c_str());
        if(message.length() > 100) waitforline(3000);
        else waitforline(1000);
        if(rb.numLines() >= 1)
        {
            res = rb.getline();
            if(res == "") return true;
            if(res == "?")
            {
                res = message + " :NAK";
                if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + ", " + res.toStdString().c_str(),2000);
                else sb->showMessage(res.toStdString().c_str(),2000);
                return false;
            }
        }
    }
    // Here if the message transaction timmed out
    reopenSerialPort();
    res = message + " :Timeout";
    if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + ", " + res.toStdString().c_str(),2000);
    else sb->showMessage(res.toStdString().c_str(),2000);
    return true;
}

QString Comms::SendMessage(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return SendMessage(message);
    return "";
}

QString Comms::SendMess(QString name, QString message)
{
    if((name == MIPSname) || (name == "")) return SendMessage(message);
    return "";
}

void Comms::waitforline(int timeout)
{
//    rb.waitforline(timeout);
//    return;

    QTime timer;

    if(timeout == 0)
    {
        while(1)
        {
            readData2RingBuffer();
            if(rb.numLines() > 0) break;
        }
        return;
    }
    timer.start();
    while(timer.elapsed() < timeout)
    {
        readData2RingBuffer();
        if(rb.numLines() > 0) break;
    }
}

QString Comms::SendMessage(QString message)
{
    QString res;

    if (!serial->isOpen() && !client.isOpen())
    {
        if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + " SM Disconnected!",2000);
        else sb->showMessage("SM Disconnected!",2000);
        return "";
    }
    if(client.isOpen()) keepAliveTimer->setInterval(600000);
    for(int i=0;i<2;i++)
    {
        rb.clear();
        if (serial->isOpen())
        {
            QString m;
            if(message.length() > 100) for(int j=0;j<message.length();j++)
            {
                m = message.at(j);
                serial->write(m.toStdString().c_str());
                if(((j+1) % 100) == 0) msDelay(10);
            }
            else serial->write(message.toStdString().c_str());
        }
        if (client.isOpen()) client.write(message.toStdString().c_str());
        if(message.length() > 100) waitforline(3000);
        else waitforline(1000);
        if(rb.numLines() >= 1)
        {
            res = rb.getline();
            if(res != "") return res;
        }
    }
    // Here if the message transaction timmed out
    reopenSerialPort();
    res = message + " :Timeout";
    if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + ", " + res.toStdString().c_str(),2000);
    else sb->showMessage(res.toStdString().c_str(),2000);
    res = "";
    return res;
}

QString Comms::SendMess(QString message)
{
    return SendMessage(message);
}

// This function is called after a connection to MIPS is established. The function
// reads the MIPS name, its version and optionally sets the MIPS time and date based
// on version number.
// The function also looks in the app dir for a file that contains initalization
// commands and sends them to the MIPS system. The file name is "MIPS name".ini
void Comms::GetMIPSnameAndVersion(void)
{
    MIPSname = SendMessage("GNAME\n");
    // Get the version string
    QStringList reslist = SendMessage("GVER\n").split(" ");
    major = minor = 1;
    if(reslist.count() > 2)
    {
        QStringList verlist = reslist[1].split(".");
        if(verlist.count() == 2)
        {
            major = verlist[0].toInt();
            bool ok;
            for(int i=5;i>0;i--)
            {
                minor = verlist[1].left(i).toInt(&ok);
                if(ok) break;
            }
        }
    }
    if((major == 1) && (minor >= 201))
    {
        QString str = QDateTime::currentDateTime().time().toString();
        SendCommand("STIME," + str + "\n");
        str = QDateTime::currentDateTime().date().toString("dd/MM/yyyy");
        SendCommand("SDATE," + str + "\n");
    }
    // Open the ini file in the app dir and send data to MIPS.
#ifdef Q_OS_MAC
    QString ext = ".app";
#else
    QString ext = ".exe";
#endif
    int i = QApplication::applicationFilePath().indexOf(QApplication::applicationName() + ext);
    if(i == -1) return;
    QString FileName = QApplication::applicationFilePath().left(i) + QApplication::applicationName() + ".ini";
    QFile file(FileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream stream(&file);
        QString line;
        do
        {
            line = stream.readLine();
            qDebug() << line;
            if(line.trimmed().startsWith("#")) continue;
            if(line.trimmed() == "") continue;
            // If here set the line to MIPS
            SendCommand(line + "\n");
        }while(!line.isNull());
        file.close();
        rb.clear();
    }
}

// Open connection to MIPS and read its name.
bool Comms::ConnectToMIPS()
{
    QTime timer;

    MIPSname = "";
    if(client.isOpen() || serial->isOpen()) return true;

    if(host != "")
    {
       client_connected = false;
       client.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
       client.connectToHost(host, 2015);
       sb->showMessage(tr("Connecting..."));
       timer.start();
       while(timer.elapsed() < 30000)
       {
           QApplication::processEvents();
           if(client_connected)
           {
               keepAliveTimer->start(600000);
               GetMIPSnameAndVersion();
               return true;
           }
       }
       sb->showMessage(tr("MIPS failed to connect!"));
       client.abort();
       client.close();
       return false;
    }
    else
    {
       openSerialPort();
       serial->setDataTerminalReady(true);
       GetMIPSnameAndVersion();
    }
    return true;
}

void Comms::DisconnectFromMIPS()
{
    if(client.isOpen())
    {
       client.close();
       keepAliveTimer->stop();
       reconnectTimer->stop();
    }
    closeSerialPort();
}

bool Comms::isAMPS(QString port, QString baud)
{
    QString res;

    if(serial->isOpen()) return false;  // If the port is open then return false
    disconnect(serial, SIGNAL(error(QSerialPort::SerialPortError)),0,0);
    // Init the port parameters
    p.name = port;
    serial->setPortName(p.name);
#if defined(Q_OS_MAC)
    serial->setPortName("cu." + p.name);
#endif
    if(serial->isOpen()) return false;
    p.baudRate = baud.toInt();
    p.dataBits = QSerialPort::Data8;
    p.parity = QSerialPort::EvenParity;
    p.stopBits = QSerialPort::TwoStop;
    p.flowControl = QSerialPort::SoftwareControl;
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite))
    {
        serial->setDataTerminalReady(true);  // Required on PC but not on a MAC
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        SendMessage("\n");   // Flush anything out!
        rb.clear();
        res = SendMessage("GVER\n");
        SendString("RTM,ON\n");
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        serial->close();
        serial->close();
        serial->close();
        serial->close();
        serial->clearError();
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        rb.clear();
        if(res.startsWith("V")) return true;
    }
    serial->clearError();
    rb.clear();
    return false;
}

bool Comms::isMIPS(QString port)
{
    QString res;

    if(serial->isOpen()) return false;  // If the port is open then return false
    disconnect(serial, SIGNAL(error(QSerialPort::SerialPortError)),0,0);
    // Init the port parameters
    p.name = port;
    serial->setPortName(p.name);
#if defined(Q_OS_MAC)
    serial->setPortName("cu." + p.name);
#endif
    if(serial->isOpen()) return false;
    p.baudRate = QSerialPort::Baud115200;
    p.dataBits = QSerialPort::Data8;
    p.parity = QSerialPort::NoParity;
    p.stopBits = QSerialPort::OneStop;
    p.flowControl = QSerialPort::NoFlowControl;
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite))
    {
        serial->setDataTerminalReady(true);  // Required on PC but not on a MAC
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        res = SendMessage("GVER\n");
        SendString("ECHO,FALSE\n");
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        serial->close();
        serial->close();
        serial->close();
        serial->close();
        serial->clearError();
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        rb.clear();
        if(res.contains("Version")) return true;
    }
    serial->clearError();
    rb.clear();
    return false;
}

void Comms::msDelay(int ms)
{
    QTime timer;

    timer.start();
    while(timer.elapsed() < ms) QApplication::processEvents();
}

void Comms::reopenSerialPort(void)
{
    if(!serial->isOpen()) return;
    serial->close();
    msDelay(250);
    serial->open(QIODevice::ReadWrite);
    serial->setDataTerminalReady(true);
}

void Comms::reopenPort(void)
{
    QTime timer;

    if(host != "")
    {
        client_connected = false;
        client.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        client.connectToHost(host, 2015);
        sb->showMessage(tr("Connecting..."));
        timer.start();
        while(timer.elapsed() < 5000)
        {
            QApplication::processEvents();
            if(client_connected) return;
        }
        sb->showMessage(tr("MIPS failed to connect!"));
        client.abort();
        client.close();
        return;
    }
    else
    {
        serial->close();
        msDelay(250);
        serial->open(QIODevice::ReadWrite);
        serial->setDataTerminalReady(true);
    }
}

bool Comms::openSerialPort()
{
    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,SLOT(handleError(QSerialPort::SerialPortError)));
    serial->setPortName(p.name);
    #if defined(Q_OS_MAC)
        serial->setPortName("cu." + p.name);
    #endif
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite))
    {
        sb->showMessage(QString("Connected to %1 : %2, %3, %4, %5, %6")
                               .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                               .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        return true;
    }
    else
    {
        QMessageBox::critical(this, QString("Error"), serial->errorString());
        sb->showMessage(tr("Open error: ") + serial->errorString());
    }
    return false;
}

void Comms::closeSerialPort()
{
    if (serial->isOpen()) serial->close();
    serial->close();
    serial->close();
    serial->close();
    if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + " Closed!",2000);
    else sb->showMessage("Closed!",2000);
    disconnect(serial, SIGNAL(error(QSerialPort::SerialPortError)),0,0);
}

void Comms::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        QThread::sleep(1);
        closeSerialPort();
        if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + tr(" Critical Error, port closing: ") + serial->errorString());
        else sb->showMessage(tr("Critical Error, port closing: ") + serial->errorString());
        if(properties != NULL)
        {
            if(properties->AutoRestore)
            {
                reconnectTimer->setInterval(2000);
                reconnectTimer->start();
            }
        }
    }
}

void Comms::writeData(const QByteArray &data)
{
    if(client.isOpen())
    {
        for(int i=0;i<data.length();i++)
        {
            client.putChar(data.at(i));
            if(((i+1) % 100) == 0) msDelay(10);
        }
        //client.write(data);
    }
    if(serial->isOpen())
    {
        for(int i=0;i<data.length();i++)
        {
            serial->putChar(data.at(i));
            if(((i+1) % 100) == 0) msDelay(10);
        }
        //serial->write(data);
    }
}

void Comms::readData2RingBuffer(void)
{
    int i;

    if(client.isOpen())
    {
        if(client.waitForReadyRead(1) || (client.bytesAvailable() > 0))
        {
           QByteArray data = client.readAll();
           for(i=0;i<data.size();i++) rb.putch(data[i]);
        }
    }
    if(serial->isOpen())
    {
        if(serial->waitForReadyRead(1) || (serial->bytesAvailable() > 0))
        {
           QByteArray data = serial->readAll();
           for(i=0;i<data.size();i++) rb.putch(data[i]);
        }
    }
    emit DataReady();
}

void Comms::connected(void)
{
    if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + tr(" MIPS connected"));
    else sb->showMessage(tr("MIPS connected"));
    client_connected = true;
}

bool Comms::isConnected(void)
{
    if(client_connected) return true;
    if(serial->isOpen()) return true;
    return false;
}

void Comms::disconnected(void)
{
    if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + " Disconnect signaled!",2000);
    else sb->showMessage("Disconnect signaled!!",2000);
}

QString Comms::getline(void)
{
    return(rb.getline());
}

QByteArray Comms::readall(void)
{
    QByteArray data;
    char c;

    while(1)
    {
      c = rb.getch();
      if(c==0) return data;
      data += c;
    }
}

void Comms::slotAboutToClose(void)
{
}

void Comms::slotKeepAlive(void)
{
   SendString("\n");
}

void Comms::slotReconnect(void)
{
    if(!serial->isOpen())
    {
        serial->open(QIODevice::ReadWrite);
        serial->setDataTerminalReady(true);
        connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,SLOT(handleError(QSerialPort::SerialPortError)));
    }
    if(serial->isOpen())
    {
        reconnectTimer->stop();
        if(!MIPSname.isEmpty()) sb->showMessage(MIPSname + tr(" Serial port reconnected!"));
        else sb->showMessage(tr("Serial port reconnected!"));
    }
}
