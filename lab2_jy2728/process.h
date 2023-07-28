#ifndef PROCESS_H
#define PROCESS_H
#include<iostream>

class Process {
    public:
    int arrivalTime;
    int totalCPUTime;
    int CPUBurst;
    int IOBurst;

    int pid;
    int static_prio;
    int dynam_prio;
    int cur_cb; //current cpu burst
    int cur_io;
    int stateTimeStamp;
    enum STATE {STATE_CREATED, STATE_READY,
        STATE_RUNNING, STATE_BLOCKED
    } cur_state = STATE_CREATED;

    private:
    int time_running = 0;
    int time_blocked = 0;
    int time_ready = 0;

    public:
    Process(int pid, int AT, int TC, int CB, int IO, int stat_prio);
    ~Process();

    /*void done_waiting(int cur_time);
    void start_running(int cur_time, int new_cpu_burst);
    */

    void ready_to_running(int cur_time, int new_cpu_burst);

    void done_running(int cur_time);
    void start_blocked(int cur_time, int new_io_burst);

    void runnning_to_ready(int cur_time);
    void blocked_to_ready(int cur_time);
    void created_to_ready(int cur_time);
    void reset_dynam_prio();
    bool complete();

    int get_cpu_time();
    int get_io_time();
    int get_wait_time();

};
#endif