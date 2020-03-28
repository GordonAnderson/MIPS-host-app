#include "ringbuffer.h"
#include <QDebug>


RingBuffer::RingBuffer(void)
{
    clear();
}

void RingBuffer::clear(void)
{
    head = 0;
    tail = 0;
    count = 0;
    lines = 0;
}

void RingBuffer::waitforline(int timeout = 0)
{
    QTime timer;

    if(timeout == 0)
    {
        while(1)
        {
            if(lines > 0) break;
            QApplication::processEvents();
        }
        return;
    }
    timer.start();
    while(timer.elapsed() < timeout)
    {
        if(lines > 0) break;
        QApplication::processEvents();
    }
}

int RingBuffer::size(void)
{
    return count;
}

int  RingBuffer::available(void)
{
    return (rbSIZE-count);
}

int RingBuffer::numLines(void)
{
    return lines;
}

char RingBuffer::getch(void)
{
    char c;

    if(count == 0) return(0);
    c = buffer[tail++];
    if(tail >= rbSIZE) tail = 0;
    count--;
    if(c == '\n')
    {
        lines--;
    }
    return c;
}

int RingBuffer::putch(char c)
{
    if(c == 0x06) return(count);
    if(c == 0x15) return(count);
    if(c == '\r') return(count);        // ignore \r
    if(count >= rbSIZE) return(-1);
    if(c == '\n')
    {
        lines++;
    }
    buffer[head++] = c;
    if(head >= rbSIZE) head = 0;
    count++;
    return(count);
}

QString RingBuffer::getline(void)
{
    QString str="";
    char c;

    if(lines <= 0) return str;
    while(1)
    {
        c = getch();
        if(c == '\n') break;
        if(count <= 0) break;
        str += c;
    }
    return str;
}
