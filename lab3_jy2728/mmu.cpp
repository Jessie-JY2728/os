#include<iostream>
#include<sstream>
#include<queue>
#include<limits.h>
#include<unistd.h>
#include "Pager.h"
#include "mmu.h"

using namespace std;

/**********************************/
/* globals                        */
/**********************************/

// unit cost for each operation
const int COST_RW = 1;
const int COST_CTX_SWITCH = 130;
const int COST_PROC_EXIT = 1230;
const int COST_MAP = 350;
const int COST_UNMAP = 410;
const int COST_IN = 3200;
const int COST_OUT = 2750;
const int COST_FIN = 2350;
const int COST_FOUT = 2800;
const int COST_ZERO = 150;
const int COST_SEGV = 440;
const int COST_SEGPROT = 410;

// summaries
unsigned long INST_COUNT = 0;
unsigned long CTX_SWITCHES = 0;
unsigned long PROC_EXITS = 0;
unsigned long long TOTAL_COST = 0;

// print options
bool option_O = false;
bool option_P = false;
bool option_F = false;
bool option_S = false;
bool option_x = false;
bool option_f = false;

Pager* pager;
vector<process*> processes;

ifstream input;
int num_frames = 0;
int num_processes = 0;
process* cur_process = nullptr;
queue<int> free_pool;   // all free frames stored by index in frame table


/**********************************/
/* pagers go here                 */
/**********************************/

FIFO::FIFO(int num_frames) {
    total_frames = num_frames;
    hand = 0;
    for (int i = 0; i < num_frames; i++) {
        frame_table[i].frame_idx = i;
    }
}

frame_t* FIFO::select_victim_frame() {
    //adv_hand(hand);
    int h = hand;
    hand = (hand+1) % total_frames;
    return &(frame_table[h]);
}

Clock::Clock(int num_frames) {
    total_frames = num_frames;
    hand = 0;
    for (int i = 0; i < num_frames; i++)
        frame_table[i].frame_idx = i;
}

frame_t* Clock::select_victim_frame() {
    frame_t* frame = nullptr;

    while (true) {
        frame = &(frame_table[hand]);
        pte_t &pte = ((processes[frame->pid])->page_table)[frame->vpage];
        hand = (hand+1) % total_frames;
        if (pte.referenced) pte.referenced = 0;
        else return frame;
    }
}

Random::Random(int num_frames, string rfile) {
    rand.open(rfile);
    if (!rand.is_open()) {
        cout << "rand file not found" << endl;
        exit(1);
    }
    total_frames = num_frames;
    read_count = 0;
    rand >> total_rand;

    for (int i = 0; i < num_frames; i++)
        frame_table[i].frame_idx = i;
}

Random::~Random() {
    rand.close();
}

frame_t* Random::select_victim_frame() {
    int h;
    rand >> h;
    read_count++;
    if (read_count == total_rand) {
        rand.clear();
        rand.seekg(0, ios::beg);
        read_count = 0;
        int dump;
        rand >> dump;
    }
    return &(frame_table[h % total_frames]);
}

ESC::ESC(int num_frames) {
    total_frames = num_frames;
    hand = 0;
    last_reset = 0;
    for (int i = 0; i < num_frames; i++) {
        frame_table[i].frame_idx = i;
    }
}

frame_t* ESC::select_victim_frame() {
    bool need_reset = false;
    if (INST_COUNT - last_reset >= RESET_RANGE) {
        need_reset = true;
        last_reset = INST_COUNT;
    }

    frame_t* cur_frame;
    int h = hand;
    int min_class = 3;
    int min_idx = h;
    do {
        cur_frame = &(frame_table[h]);
        pte_t &pte = ((processes[cur_frame->pid])->page_table)[cur_frame->vpage];
        int cur_class = 2 * pte.referenced + pte.modified;
        if (need_reset) pte.referenced = 0;
        if (cur_class < min_class) {
            min_class = cur_class;
            min_idx = h;
        }
        if (min_class == 0 && !need_reset) break;
        h = (h+1) % total_frames;
    } while (h != hand);
    hand = (min_idx + 1) % total_frames;
    return &(frame_table[min_idx]);
}

Aging::Aging(int num_frames) {
    total_frames = num_frames;
    hand = 0;
    for (int i = 0; i < num_frames; i++) {
        frame_table[i].frame_idx = i;
        frame_table[i].counter = 0;
    }
}

void Aging::reset_counter(frame_t* frame, unsigned long age) {
    frame->counter = 0;
}

frame_t* Aging::select_victim_frame() {
    int h = hand;
    frame_t* cur_frame;
    frame_t* victim = &(frame_table[h]);

    do
    {
        cur_frame = &(frame_table[h]);
        pte_t &pte = ((processes[cur_frame->pid])->page_table)[cur_frame->vpage];
        cur_frame->counter = cur_frame->counter >> 1;
        if (pte.referenced) {
            cur_frame->counter = (cur_frame-> counter | 0x80000000);
            pte.referenced = 0;
        }

        if (cur_frame->counter < victim->counter) {
            victim = cur_frame;
        }
        h = (h+1) % total_frames;
    } while (h != hand);
    hand = (victim->frame_idx+1) % total_frames;
    return victim;  
}

WSet::WSet(int num_frames) {
    total_frames = num_frames;
    hand = 0;
    for (int i = 0; i < num_frames; i++) {
        frame_table[i].frame_idx = i;
        frame_table[i].counter = 0;
    }
}

void WSet::reset_counter(frame_t* frame, unsigned long age) {
    frame->counter = age;
}


frame_t* WSet::select_victim_frame() {
    int h = hand;
    frame_t* cur_frame;
    int min_idx = -1;
    unsigned long min_time = ULONG_MAX;

    do
    {
        cur_frame = &(frame_table[h]);
        pte_t &pte = ((processes[cur_frame->pid])->page_table)[cur_frame->vpage];

        if (pte.referenced) {
            pte.referenced = 0;
            cur_frame->counter = INST_COUNT;
        } else {
            unsigned long diff = INST_COUNT - cur_frame->counter;
            if (diff > TAU) {
                min_idx = cur_frame->frame_idx;
                break;
            }
        }
        min_idx = (cur_frame->counter < min_time) ? cur_frame->frame_idx : min_idx;
        min_time = (cur_frame->counter < min_time) ? cur_frame->counter : min_time; 
        h = (h+1) % total_frames;
    } while (h != hand);

    hand = (min_idx + 1) % total_frames;
    return &(frame_table[min_idx]);
}

/**********************************/
/* process class starts here      */
/**********************************/

process::process(int pid, int vma_count)
{
    this->pid = pid;
    this->vmas.reserve(vma_count);
}

int process::in_vma(int vpage) {
    for (VMA vma : vmas) {
        if (vma.start <= vpage && vma.end >= vpage)
            return vma.idx;
    }
    return -1;
}

bool process::page_legal(int vpage) {
    if (page_table[vpage].vma_checked) {
        return 1 ^ (page_table[vpage].vma_section >> 3);
    } else {
        page_table[vpage].vma_checked = 1;
        int vmaidx = in_vma(vpage);
        if (vmaidx == -1) {
            page_table[vpage].vma_section = 0b1111;
            return false;
        }
        page_table[vpage].vma_section = vmaidx;
        return true;
    }
}

bool process::w_protected(int vpage) {
    if (page_table[vpage].wp_checked) {
        return page_table[vpage].w_protected;
    } else {
        if (page_table[vpage].vma_checked) {
            int vma_sec = (int) page_table[vpage].vma_section;
            if (vmas[vma_sec].w_protected) page_table[vpage].w_protected = 1;
        } else {
            int vma_sec = in_vma(vpage);
            page_table[vpage].vma_checked = 1;
            page_table[vpage].vma_section = vma_sec;
            if (vmas[vma_sec].w_protected) page_table[vpage].w_protected = 1;
        }
        page_table[vpage].wp_checked = 1;
        return page_table[vpage].w_protected;
    }
}

bool process::file_mapped(int vpage) {
    if (page_table[vpage].fm_checked) {
        return page_table[vpage].file_mapped;
    } else {
        if (page_table[vpage].vma_checked) {
            int vma_sec = (int) page_table[vpage].vma_section;
            if (vmas[vma_sec].file_mapped) page_table[vpage].file_mapped = 1;
        } else {
            int vma_sec = in_vma(vpage);
            page_table[vpage].vma_checked = 1;
            page_table[vpage].vma_section = vma_sec;
            if (vmas[vma_sec].file_mapped) page_table[vpage].file_mapped = 1;
        }
        page_table[vpage].fm_checked = 1;
        return page_table[vpage].file_mapped;
    }
}



/**********************************/
/* mmu simulation goes here       */
/**********************************/

frame_t* allocate_frame_from_free_list() {
    if (free_pool.empty()) return nullptr;
    int rv = free_pool.front();
    free_pool.pop();
    return &(pager->frame_table[rv]);
}

frame_t* get_frame() {
    frame_t* frame = allocate_frame_from_free_list();
    if (frame != nullptr) {
        return frame;
    }
    frame = pager->select_victim_frame(); 
    process* proc = processes[frame->pid];
    pte_t* pte = &(proc->page_table[frame->vpage]);
    pte->valid = 0;
    if (option_O) //cout << " UNMAP " << frame->pid << ":" << frame->vpage << endl;
        printf(" UNMAP %d:%d\n", frame->pid,  frame->vpage);
    proc->unmaps += 1;  // one more unmap
    TOTAL_COST += COST_UNMAP;

    if (pte->modified) {
        if (proc->file_mapped(frame->vpage)) {
            if (option_O) //cout << " FOUT" << endl;
                printf(" FOUT\n");
            proc->fouts += 1;
            TOTAL_COST += COST_FOUT;
        }
        else {
            if (option_O) // cout << " OUT" << endl;
                printf(" OUT\n");
            proc->outs += 1;
            pte->paged_out = 1;
            TOTAL_COST += COST_OUT;
        }
    }
    frame->pid = -1;
    frame->vpage = -1;
    return frame;
}



void initialize() {
    string line;
    while (getline(input, line)) {
        while (line.empty() || line[0] == '#') {
            getline(input, line);
        }
        stringstream ss(line);
        ss >> num_processes;
        break;
    }

    for (int i = 0; i < num_processes; i++) {
        getline(input, line);
        while (line.empty() || line[0] == '#') {
            getline(input, line);
            continue;
        }
        int num_vma;
        stringstream ss(line);
        ss >> num_vma;
        process* proc = new process(i, num_vma);
        for (int j = 0; j < num_vma; j++) {
            getline(input, line);
            while (line.empty() || line[0] == '#') {
                getline(input, line); 
            }
            stringstream ss(line);
            int start, end; 
            bool wp, fm;
            ss >> start >> end >> wp >> fm;
            VMA vma;
            vma.idx = j;
            vma.start = start;
            vma.end = end;
            vma.file_mapped = fm;
            vma.w_protected = wp;
            proc->vmas.push_back(vma);
        }
        processes.push_back(proc);
    }
}


bool get_next_instr(char &opcode, int &vpage) {
    if (input.eof()) return false;

    string line;
    getline(input, line);
    while(line.empty() || line[0] == '#') {
        if (input.eof()) {
            opcode = ' ';
            vpage = -1;
            return false;
        } else {
            getline(input, line);
        }
    }
    stringstream ss(line);
    ss >> opcode >> vpage;
    if (option_O) // cout << INST_COUNT << ": ==> " << opcode << " " << vpage << endl;
        printf("%lu: ==> %c %d\n", INST_COUNT, opcode, vpage);
    INST_COUNT++;
    return true;
}


void simulate() {
    char opcode = ' ';
    int vpage = -1;
    while (get_next_instr(opcode, vpage)) {

        if (opcode == 'c') {
            cur_process = processes[vpage];
            CTX_SWITCHES++;
            TOTAL_COST += COST_CTX_SWITCH;
            continue;
        }
        if (opcode == 'e') {
            printf("EXIT current process %d\n", cur_process->pid);

            for (int i = 0; i < MAX_VPAGES; i++) {
                pte_t &pte = cur_process->page_table[i];
                if (pte.valid) {
                    pte.valid = 0;
                    if (option_O) //cout << " UNMAP " << cur_process->pid << ":" << i << endl;
                        printf(" UNMAP %d:%d\n", cur_process->pid, i);
                    TOTAL_COST += COST_UNMAP;
                    cur_process->unmaps += 1;

                    if (cur_process->file_mapped(i) && pte.modified) {
                        if (option_O) //cout << " FOUT" << endl;
                            printf(" FOUT\n");
                        cur_process->fouts += 1;
    
                        TOTAL_COST += COST_FOUT;
                    }
                    free_pool.push(pte.frame_idx);
                    pager->frame_table[pte.frame_idx].vpage = -1;
                }
                pte.paged_out = 0;
            }
            PROC_EXITS++;
            TOTAL_COST += COST_PROC_EXIT;
            continue;
        }

        TOTAL_COST += COST_RW;  // this can only be rw

        if (!cur_process->page_legal(vpage)) {
            if (option_O) //cout << " SEGV" << endl;
                printf(" SEGV\n");
            cur_process->segvs += 1;
            TOTAL_COST += COST_SEGV;
            continue;
        }
        pte_t* pte = &(cur_process->page_table[vpage]);
        if (!pte->valid) {
            frame_t* newframe = get_frame();
            if (cur_process->file_mapped(vpage)) {
                if (option_O) // cout << " FIN" <<endl;
                    printf(" FIN\n");
                cur_process->fins += 1;
                TOTAL_COST += COST_FIN;
            }
            else if (cur_process->page_table[vpage].paged_out) {
                if (option_O) //cout << " IN" << endl;
                    printf(" IN\n");
                cur_process->ins += 1;
                TOTAL_COST += COST_IN;
            }
            else { 
                if (option_O) //cout << " ZERO" << endl;
                    printf(" ZERO\n");
                cur_process->zeros += 1;
                TOTAL_COST += COST_ZERO;
            }
            pte->modified = 0;
            pte->fm_checked = 0;
            pte->wp_checked = 0;
            pte->vma_checked = 0;

            pte->valid = 1;
            pte->frame_idx = newframe->frame_idx;
            if (option_O) //cout << " MAP "<< newframe->frame_idx << endl;
                printf(" MAP %d\n", newframe->frame_idx);
            pager->reset_counter(newframe, INST_COUNT);
            cur_process->maps += 1;
            TOTAL_COST += COST_MAP;

            newframe->pid = cur_process->pid;
            newframe->vpage = vpage;
        }

        pte->referenced = 1;
        
        if (opcode == 'w') {
            if (cur_process->w_protected(vpage)) {
                cur_process->segprots += 1;
                if (option_O) //cout << " SEGPROT" << endl;
                    printf(" SEGPROT\n");
                TOTAL_COST += COST_SEGPROT;
            } else {
                pte->modified = 1;
            }
        }      
    }
}


void dummy_report() {
    if (option_P) {
        for (process* p : processes) {
            printf("PT[%d]:", p->pid);
            for (int i = 0; i < MAX_VPAGES; i++) {
                pte_t &entry = p->page_table[i];
                if (entry.valid) {
                    printf(" %d:", i);
                    printf(entry.referenced ? "R" : "-");
                    printf(entry.modified ? "M" : "-");
                    printf(entry.paged_out ? "S" : "-" );
                } else {
                    printf(entry.paged_out ? " #" : " *");
                }
            }
            printf("\n");
        }
    }

    if (option_F) {
        printf("FT:");
        for (int i = 0; i < num_frames; i++) {
            frame_t &frame = pager->frame_table[i];
            if (frame.vpage != -1)
                printf(" %d:%d", frame.pid, frame.vpage);
            else 
                printf(" *");
        }
        printf("\n");
    }

    if (option_S) {
        for (process* p : processes) {
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    p->pid, p->unmaps, p->maps, p->ins, p->outs, p->fins, p->fouts,
                    p->zeros, p->segvs, p->segprots);

        }
        printf("TOTALCOST %lu %lu %lu %llu %lu\n", 
                INST_COUNT, CTX_SWITCHES, PROC_EXITS, TOTAL_COST, sizeof(pte_t));
    }
}


/**********************************/
/* main & parsing go here         */
/**********************************/

int main(int argc, char* argv[]) {
    string infile = argv[argc - 2];
    string rfile = argv[argc - 1];
    char algo = ' ';

    int c = getopt(argc, argv, "f:a:o:");
    string o_arg;
    while (c != -1) {
        switch (c)
        {
        case 'f':
            sscanf(optarg, "%d", &num_frames);
            break;
        case 'a':
            algo = optarg[0];
            break;
        case 'o':
            o_arg = optarg;
            //cout << optarg[0] << optarg[1] << optarg[2] << optarg[3]<< endl;
            for (int i = 0; i < o_arg.length(); i++) {
                switch(o_arg[i]) {
                    case 'O':
                        option_O = true;
                        break;
                    case 'P':
                        option_P = true;
                        break;
                    case 'F':
                        option_F = true;
                        break;
                    case 'S':
                        option_S = true;
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
        }
        c = getopt(argc, argv, "f:a:o:");
    }
    if (num_frames <= 0) {
        cout << "Really funny .. you need at least one frame" << endl;
        exit(1);
    }
    for (int i = 0; i < num_frames; i++) {
        free_pool.push(i);  // initially all frames are free
    }

    input.open(infile);
    if (!input.is_open()) {
        cout << "Input file not open."<<endl;
        exit(1);
    }

    switch (algo)
    {
    case 'f':
        pager = new FIFO(num_frames);
        break;
    case 'c':
        pager = new Clock(num_frames);
        break;
    case 'r':
        pager = new Random(num_frames, rfile);
        break;
    case 'e':
        pager = new ESC(num_frames);
        break;
    case 'a':
        pager = new Aging(num_frames);
        break;
    case 'w':
        pager = new WSet(num_frames);
    default:
        break;
    }

    initialize();
    simulate();
    dummy_report();

    input.close();
}

