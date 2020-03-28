#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QByteArray>
#include <QString>
#include <QTime>
#include <QApplication>
#include <QtNetwork/QTcpSocket>
#include <QtSerialPort/QSerialPort>

#define rbSIZE 10000

class RingBuffer : public QObject
{
    Q_OBJECT

public:
    explicit RingBuffer(void);
    char getch(void);
    int  putch(char c);
    int  size(void);
    int  available(void);
    int  numLines(void);
    void clear(void);
    void waitforline(int);
    QString getline(void);

protected:

private:
    int     buffer[rbSIZE];
    int     head;
    int     tail;
    int     count;
    int     lines;
};

#endif // RINGBUFFER_H

