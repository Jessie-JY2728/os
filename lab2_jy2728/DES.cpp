#include<iostream>
#include<fstream>
#include<deque>
#include<sstream>
#include<unistd.h>
#include "event.h"
#include "scheduler.h"
using namespace std;

deque<Event*> eventQueue;
vector<Process*> procTable;
Scheduler* scheduler;
string sched_name;
bool call_sched = false;
bool HASPRIO = false;
bool PREEMPT = false;
Process* curRunningProc = nullptr;
//int cpu_busy_time = 0; //total time that cpu is busy by all processes
int io_count = 0;   // count of processes doing IO, +1 when trans_to_block, -1 when finish block
int io_start = 0;   // timestamp of the latest occurence when io_count goes from 0 to 1
int io_busy_time = 0;  // total time that io is busy for any process

int current_time = 0;   //current time is retrieved from event.timestamp
int max_priority = 4;   //max priority, default 4
int quantum = 10000;    // quantum, time for preempt, default 10k
ifstream input_file; //input file
ifstream rand_file; // random numbers
int RC;
int rc = 0;

bool vflag = false; //verbose flag


Event* get_event() {
    Event* rv = nullptr;
    if (!eventQueue.empty()) {
        rv = eventQueue.front();
        eventQueue.erase(eventQueue.begin());
    }
    return rv;
}

void put_event(Event* evt) {
    auto left = eventQueue.begin();
    auto right = eventQueue.end();
    while (left != right) {
        auto mid = left + (right - left) / 2;
        if ((*mid)->timeStamp > evt->timeStamp) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    eventQueue.insert(left, evt);
}


void rm_event() {   //flag create time as -1 to nullify the event
    for (Event* e : eventQueue) {
        if (e->proc == curRunningProc) {
            e->createTime = -1;
        }
    }
}

int get_next_event_time() {
    if (eventQueue.empty()) {
        return -1;
    } else {
        return eventQueue.front()->timeStamp;
    }
}

int myrandom(int burst) {
    int num;
    rand_file >> num;
    cout<<num<<" ";
    int rv = 1 + (num % burst);
    rc++;
    //rewind if file ends
    if (rc == RC) {
        rand_file.clear();
        rand_file.seekg(0, ios::beg);
        rand_file >> num;
        rc = 0;
        cout<<endl;
    }
    return rv;
}

void initialize() {
    rand_file >> RC;
    string dump;
    int AT, TC, CB, IO;
    int count = 0;
    while(getline(input_file, dump)) {
        if (dump.length() == 0) continue;
        stringstream line(dump);
        line>>AT>>TC>>CB>>IO;
        Process* proc = new Process(count, AT, TC, CB, IO, myrandom(max_priority));
        count++;
        procTable.push_back(proc);
        Event* evt = new Event(proc->arrivalTime, proc->arrivalTime, proc, Event::TO_READY);
        eventQueue.push_back(evt);
    }
    current_time = procTable[0]->stateTimeStamp;
}

bool preprio_cond2() {
    for (Event* evt : eventQueue) {
        if (evt->proc == curRunningProc && evt->timeStamp == current_time)
            return evt->createTime == -1;
    }
    return true;
}

void simulate() {
    Event* evt;
    while(!eventQueue.empty()) {
        evt = get_event();
        if (evt->createTime == -1) {    //if true, then event has been nullified by a preempt
            delete evt;
            evt = nullptr;
            continue;
        }

        Process* process = evt->proc;
        current_time = evt->timeStamp;
        Event::TRANS trans = evt->transition;
        int timeInPrevState = current_time - process->stateTimeStamp;
        delete evt;
        evt = nullptr;

        if (vflag)
            cout<< current_time << " "<< process->pid <<" "<< timeInPrevState<<": ";
        switch (trans)
        {
        case Event::TO_READY:
            call_sched = true;
            switch (process->cur_state)
            {
            case Process::STATE_CREATED:  //initial
                process->created_to_ready(current_time);
                if (vflag)
                    cout<<"CREATED -> READY"<<endl;
                break; 
            case Process::STATE_BLOCKED: // finish IO
                process->blocked_to_ready(current_time);
                if (vflag) {
                    cout<<"BLOCKED -> READY"<<endl;
                }
                io_count--;
                if (io_count == 0)
                    io_busy_time += current_time - io_start;
                break;
            default:
                exit(1);
            }
            scheduler->add_process(process);
            if (PREEMPT && curRunningProc != nullptr) {
                bool cond1 = curRunningProc->dynam_prio < process->dynam_prio;
                bool cond2 = preprio_cond2();
                string decision = cond1 && cond2 ? "YES" : "NO";
                if (vflag) {
                    printf("    --> PrioPreempt Cond1=%d Cond2=%d (%d) --> %s\n", cond1, cond2, curRunningProc->pid, decision.c_str());
                }
                if (cond1 && cond2) {
                    rm_event();
                    Event* kill = new Event(current_time, current_time, curRunningProc, Event::TO_PREEMPT);
                    put_event(kill);
                }
            }
            break;
        case Event::TO_PREEMPT: 
            call_sched = true;
            process->runnning_to_ready(current_time);
            curRunningProc = nullptr; 
            if (vflag)
                cout<<"RUNNG -> READY  cb="<<process->cur_cb
                    <<" rem="<<process->totalCPUTime-process->get_cpu_time()
                    <<" prio="<<process->dynam_prio + 1<<endl;
            scheduler->add_process(process);
            if (!HASPRIO)
                process->reset_dynam_prio();
            break;
        case Event::TO_BLOCKED: // must come from running
            call_sched = true;
            process->done_running(current_time);
            curRunningProc = nullptr;
            if (process->complete()) {
                if (vflag)
                    cout<<"Done"<<endl;
            } else {
                io_count++;
                if (io_count == 1)
                    io_start = current_time;
                int new_io = myrandom(process->IOBurst);
                process->start_blocked(current_time, new_io);
                Event* ready_evt = new Event(current_time + new_io, current_time, process, Event::TO_READY);
                put_event(ready_evt);
                if (vflag)
                    cout<< "RUNNG -> BLOCK  ib="<<new_io<<" rem="<<process->totalCPUTime - process->get_cpu_time()<<endl;
            }
            break;
        case Event::TO_RUNNING: // ready -> running
            call_sched = true;
            int new_cpu = process->cur_cb > 0 ? process->cur_cb : myrandom(process->CPUBurst);
            process->ready_to_running(current_time, new_cpu);
            if (quantum >= process->cur_cb) {
                Event* block_evt = new Event(current_time + process->cur_cb, current_time, process, Event::TO_BLOCKED);
                put_event(block_evt);
            } else {
                Event* preempt_evt = new Event(current_time + quantum, current_time, process, Event::TO_PREEMPT);
                put_event(preempt_evt);
            }
            if (vflag) {
                cout<<"READY -> RUNNG cb="<<new_cpu<<" rem="<<process->totalCPUTime - process->get_cpu_time()
                    <<" prio="<<process->dynam_prio<<endl;
            }
            break;
        }
        
        if (call_sched) {
            if (get_next_event_time() == current_time) continue;
            call_sched = false;
            if (curRunningProc == nullptr) {
                curRunningProc = scheduler->get_next_process();
                if (curRunningProc == nullptr) continue;
                Event* runng_evt = new Event(current_time, current_time, curRunningProc, Event::TO_RUNNING);
                put_event(runng_evt);
            }
        }

    }
}

void report() {
    if (quantum < 10000)
        printf("%s %d\n", sched_name.c_str(), quantum);
    else
        printf("%s\n", sched_name.c_str());
    int last_ft = -1;
    //int first_at = procTable[0]->arrivalTime;
    int sum_cpu_time = 0;   //sum of time cpu busy
    int sum_wtg_time = 0;   //sum of time in ready state
    int sum_trt_time = 0;   //sum of turnaround time
    int num_processes = 0;
    for (Process* p : procTable) {
        num_processes++;
        printf("%04d: %4d %4d %4d %4d %1d", p->pid, p->arrivalTime, p->totalCPUTime, 
            p->CPUBurst, p->IOBurst, p->static_prio);
        int it = p->get_io_time();
        int cw = p->get_wait_time();
        int tt = p->get_cpu_time() + it + cw;
        int ft = tt + p->arrivalTime;
        if (ft > last_ft) last_ft = ft;
        sum_trt_time += tt;
        sum_wtg_time += cw;
        sum_cpu_time += p->get_cpu_time();
        printf(" | %5d %5d %5d %5d\n", ft, tt, it, cw);
    }
    double total_time = (double)last_ft;
    double cpu_util = 100.0 * (sum_cpu_time / total_time);
    double io_util = 100.0 * (io_busy_time / total_time);
    double avg_wtg = ((double) sum_wtg_time) / num_processes;
    double avg_trt = ((double) sum_trt_time) / num_processes;
    double throughput = 100.0 * (num_processes / total_time);
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", last_ft, cpu_util, io_util, avg_trt, avg_wtg, throughput);
}

int main(int argc, char* argv[]) {
    if (argc < 3)
        return 1;
    string input = argv[argc - 2];
    string rand = argv[argc - 1];
    // open files
    input_file.open(input);
    if (!input_file.is_open())
        return 1;
    rand_file.open(rand);
    if (!rand_file.is_open())
        return 1;
    char sched = 'F';
    int c = getopt(argc, argv, "vetps:");
    // parse flags;
    while (c != -1) {
        switch (c) {
            case 'v':
                vflag = true;
                break;
            case 's':
                sscanf(optarg, "%c%d:%d", &sched, &quantum, &max_priority);
                break;
            default:
                return 1;
        }
        c = getopt(argc, argv, "vetps:");
    }
    
    //TODO: setup scheduler
    switch (sched)
    {
    case 'F':
        /* code */
        sched_name = "FCFS";
        scheduler = new FCFS();
        break;
    case 'L':
        sched_name = "LCFS";
        scheduler = new LCFS();
        break;
    case 'S':
        sched_name = "SRTF";
        scheduler = new SRTF();
        break;
    case 'R':
        sched_name = "RR";
        scheduler = new FCFS();
        break;
    case 'P':
        sched_name = "PRIO";
        scheduler = new PRIO(max_priority);
        HASPRIO = true;
        break;
    case 'E':
        sched_name = "PREPRIO";
        scheduler = new PRIO(max_priority);
        HASPRIO = true;
        PREEMPT = true;
    default:
        cout<< "Unknown Scheduler spec: -v {FLSRPE}"<<endl;
        return 1;
    }

    initialize();
    simulate();
    report();
    
    return 0;

}
