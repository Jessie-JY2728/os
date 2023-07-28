#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "process.h"
#include<queue>
#include<deque>
#include<stack>
#include<vector>

class Scheduler {
    public:
    virtual void add_process(Process* p) = 0;
    virtual Process* get_next_process() = 0;
    //bool has_prio;  // true for 'E' and 'P'
};

class FCFS : public Scheduler {
    public:
    std::queue<Process*> readyQueue;
    void add_process(Process* p);
    Process* get_next_process();
    bool preempt = false;
}; 

class LCFS : public Scheduler {
    public:
    std::stack<Process*> readyStack;
    void add_process(Process* p);
    Process* get_next_process();
    bool preempt = false;    
};

class SRTF : public Scheduler {
    public:
    std::deque<Process*> readyDeque;
    void add_process(Process* p);
    Process* get_next_process();
    bool preempt = false;
};

class PRIO : public Scheduler {
    public:
    PRIO(int maxprio);
    void add_process(Process* p);
    Process* get_next_process();
    //bool has_prio = true;

    private:
    int maxIndex;
    std::vector<std::queue<Process*>> *actQueue;
    std::vector<std::queue<Process*>> *expQueue;

};
#endif