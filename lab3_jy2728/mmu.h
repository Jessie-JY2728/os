#ifndef MMU_H
#define MMU_H
#include<vector>

// max number of each kind
const int MAX_VPAGES = 64;
const int MAX_VMAS = 8;
const int MAX_PROCS = 10; 

struct pte_t
{
    unsigned valid : 1;
    unsigned referenced : 1;
    unsigned modified : 1;
    unsigned w_protected : 1;
    unsigned paged_out : 1;
    unsigned frame_idx : 7; // five required fields, 12 bits so far
    unsigned file_mapped : 1;
    unsigned wp_checked : 1;    // checked w protection with vma
    unsigned fm_checked : 1;    // checked file map with vma
    unsigned vma_checked : 1;   // 0 if the page is ref'ed 1st time by instruction, else 1
    unsigned vma_section : 4;   // VMA section index, 1111 if not belong to any section
    unsigned garbage : 12;
};

struct VMA
{
    int idx;
    int start;
    int end;
    bool w_protected;
    bool file_mapped;
};

class process
{
public:
    int pid;
    unsigned long maps = 0;
    unsigned long unmaps = 0;
    unsigned long ins = 0;
    unsigned long outs = 0;
    unsigned long fins = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segvs = 0;
    unsigned long segprots = 0;
    pte_t page_table[MAX_VPAGES];
    std::vector<VMA> vmas;

    process(int pid, int vma_count);
    ~process() = default;
    int in_vma(int vpage);  // helper to check if page is legal
    bool page_legal(int vpage); // checks if page is in one vma
    bool w_protected(int vpage);
    bool file_mapped(int vpage);
};

#endif