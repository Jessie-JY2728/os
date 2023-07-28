#include "process.h"
using namespace std;

Process::Process(int pid, int AT, int TC, int CB, int IO, int stat_prio) {
    this->pid = pid;
    arrivalTime = AT;
    totalCPUTime = TC;
    CPUBurst = CB;
    IOBurst = IO;
    static_prio = stat_prio;
    stateTimeStamp = AT;
    reset_dynam_prio();
    cur_cb = 0;
    cur_io = 0;
}

Process::~Process(){}

void Process::ready_to_running(int cur_time, int new_cpu_burst)
{
    int elapse = cur_time - stateTimeStamp;
    time_ready += elapse;
    stateTimeStamp = cur_time;
    cur_state = STATE::STATE_RUNNING;
    int rem = totalCPUTime - time_running;
    cur_cb = new_cpu_burst > rem ? rem : new_cpu_burst;
}

void Process::done_running(int cur_time)
{
    int elapse = cur_time - stateTimeStamp;
    time_running += elapse;
    cur_cb -= elapse;
}

void Process::start_blocked(int cur_time, int new_io_burst)
{
    stateTimeStamp = cur_time;
    cur_state = STATE::STATE_BLOCKED;
    cur_io = new_io_burst;
}

void Process::reset_dynam_prio() {
    dynam_prio = static_prio - 1;
}

/*void Process::running_to_blocked(int cur_time) {
    done_running(cur_time);
    stateTimeStamp = cur_time;
    cur_state = STATE::STATE_BLOCKED;
}*/

void Process::runnning_to_ready(int cur_time) {
    done_running(cur_time);
    stateTimeStamp = cur_time;
    dynam_prio--;
    cur_state = STATE::STATE_READY;
}

void Process::blocked_to_ready(int cur_time) {
    int elapse = cur_time - stateTimeStamp;
    time_blocked += elapse;
    stateTimeStamp = cur_time;
    reset_dynam_prio();
    cur_io = 0;
    cur_state = STATE::STATE_READY;
}

void Process::created_to_ready(int cur_time){
    cur_state = STATE::STATE_READY;
    stateTimeStamp = cur_time;
}

int Process::get_cpu_time() {return time_running;}
int Process::get_io_time() {return time_blocked;}
int Process::get_wait_time() {return time_ready;}

bool Process::complete() {
    return time_running == totalCPUTime;
}


/*int main() {
    Process* p = new Process(0, 100, 20, 10, 5);
    p->ready_to_running(20, 15);
    cout<< p->get_wait_time()<<" ready"<<endl;

    p->running_to_blocked(35, 5);
    cout<<p->get_cpu_time()<<" running "<<p->cur_state<<endl;

    cout<<p->dynam_prio;

}*/