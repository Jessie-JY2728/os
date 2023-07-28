#include "scheduler.h"

void FCFS::add_process(Process* p) {
    readyQueue.push(p);
}

Process *FCFS::get_next_process()
{
    if (readyQueue.empty()) {
        return nullptr;
    } else {
        Process* proc = readyQueue.front();
        readyQueue.pop();
        return proc;
    }
}

void LCFS::add_process(Process* p) {
    readyStack.push(p);
}

Process *LCFS::get_next_process() {
    if (readyStack.empty()) {
        return nullptr;
    } else {
        Process* proc = readyStack.top();
        readyStack.pop();
        return proc;
    }
}

void SRTF::add_process(Process* p)
{
    auto left = readyDeque.begin();
    auto right = readyDeque.end();
    while (left != right) {
        auto mid = left + (right - left) / 2;
        int mid_rem = (*mid)->totalCPUTime - (*mid)->get_cpu_time();
        int p_rem = p->totalCPUTime - p->get_cpu_time();
        if (mid_rem > p_rem)
            right = mid;
        else
            left = mid + 1;
    }
    readyDeque.insert(left, p);
}

Process *SRTF::get_next_process() {
    if (readyDeque.empty())
        return nullptr;
    else {
        Process* rv = readyDeque.front();
        readyDeque.erase(readyDeque.begin());
        return rv;
    }
}

PRIO::PRIO(int maxprio)
{
    maxIndex = maxprio - 1;
    actQueue = new std::vector<std::queue<Process*>>(maxprio, std::queue<Process*>());
    expQueue = new std::vector<std::queue<Process*>>(maxprio, std::queue<Process*>());
}

void PRIO::add_process(Process *p)
{
    //int prio = p->dynam_prio;
    if (p->dynam_prio == -1) {
        p->reset_dynam_prio();
        expQueue->at(maxIndex - p->dynam_prio).push(p);
    } else {
        actQueue->at(maxIndex - p->dynam_prio).push(p);
    }
}

Process *PRIO::get_next_process() {
    Process* p = nullptr;
    for (int i = 0; i <= maxIndex; i++) {
        if (actQueue->at(i).empty()) continue;
        p = actQueue->at(i).front();
        actQueue->at(i).pop();
        return p;
    }
    // not returned yet, active queue is all empty
    std::swap(actQueue, expQueue);
    for (int i = 0; i <= maxIndex; i++) {
        if (actQueue->at(i).empty()) continue;
        p = actQueue->at(i).front();
        actQueue->at(i).pop();
        return p;
    }
    return p;
}
