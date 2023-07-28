#ifndef EVENT_H
#define EVENT_H
#include "process.h"

class Event {
    public:
    int timeStamp;
    int createTime;
    Process* proc;
    enum TRANS {TO_READY, TO_PREEMPT,
        TO_RUNNING, TO_BLOCKED
    } transition;
    Event(int ts, int ct, Process* proc, TRANS t);
    ~Event();
};
#endif