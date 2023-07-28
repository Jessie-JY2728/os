#include "event.h"

Event::Event(int ts, int ct, Process *proc, TRANS t)
{
    timeStamp = ts;
    createTime = ct;
    this->proc = proc;
    transition = t;
}

Event::~Event()
{}