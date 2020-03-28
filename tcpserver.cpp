#include "tcpserver.h"

TCPserver::TCPserver(QObject *parent) :
    QObject(parent)
{
    socket = NULL;
    statusbar = NULL;
    port = 9999;
    server = new QTcpServer(this);

    // whenever a user connects, it will emit signal
    connect(server, SIGNAL(newConnection()),this, SLOT(newConnection()));
}

TCPserver::~TCPserver()
{
    if(socket != NULL) socket->close();
    server->close();
}

void TCPserver::listen(void)
{
    if(!server->listen(QHostAddress::Any, port))
    {
        if(statusbar != NULL) statusbar->showMessage("Server could not start");
    }
    else
    {
        if(statusbar != NULL) statusbar->showMessage("Server started!");
    }
}

void TCPserver::sendMessage(QString mess)
{
    if(mess == "\n") return;
    if(socket == NULL) return;
    socket->write(mess.toUtf8().constData());
    socket->flush();
}

int TCPserver::bytesAvailable(void)
{
    return(rb.size());
}

void TCPserver::newConnection(void)
{
    // need to grab the socket
    QTcpSocket *sck = server->nextPendingConnection();
    if(socket != NULL)
    {
        socket->close();
//      sck->close();
//      return;
    }
    socket = sck;
    if(socket == NULL) return;

    // connect slots to process incoming data and disconnect events
    connect(socket,SIGNAL(readyRead()),this,SLOT(readData()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(disconnected()));
}

void TCPserver::disconnected(void)
{
    disconnect(socket, SIGNAL(readyRead()), 0, 0);
    disconnect(socket, SIGNAL(disconnected()), 0, 0);
    socket->close();
    socket = NULL;
}

void TCPserver::readData(void)
{
    int space = rb.available();
    int chars = socket->bytesAvailable();
    if(space < chars) chars = space;
    if(chars <= 0) return;
    char *buffer = new char[chars];
    int cr = socket->read(buffer,chars);
    for(int i=0;i<cr;i++) rb.putch(buffer[i]);
    if(rb.size() > 0) emit dataReady();
    if(rb.numLines() > 0) emit lineReady();
}
/*
void TCPserver::readData(void)
{
    QByteArray buffer;

    buffer.append(socket->readAll());
    for(int i=0;i<buffer.count();i++) rb.putch(buffer[i]);
    if(rb.size() > 0) emit dataReady();
    if(rb.numLines() > 0) emit lineReady();
}
*/
QString TCPserver::readLine(void)
{
    if(rb.numLines() == 0) return "";
    return rb.getline();
}
