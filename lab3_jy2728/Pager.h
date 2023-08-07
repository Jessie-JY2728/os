#ifndef PAGER_H
#define PAGER_H
#include<fstream>


const int MAX_FRAMES = 128;
struct frame_t 
{
    int frame_idx;
    int pid = -1;   //process of the frame
    int vpage = -1; // virtual page the frame is reverse mapped to;
    unsigned long counter = 0;  // the counter for aging and working set
};

class Pager
{
public:
    frame_t frame_table[MAX_FRAMES];
    //Pager(int num_frames);
    int total_frames;
    virtual frame_t* select_victim_frame() = 0;
    virtual void reset_counter(frame_t* frame, unsigned long age){};
};

class FIFO : public Pager
{
    public:
    FIFO(int num_frames);
    int hand;
    frame_t* select_victim_frame();
};

class Clock : public Pager
{
    public:
    Clock(int num_frames);
    int hand;
    frame_t* select_victim_frame();
};


class Random : public Pager 
{
    public:
    std::ifstream rand;
    int read_count;
    int total_rand;
    Random(int num_frames, std::string rfile);
    ~Random();
    frame_t* select_victim_frame();
};

class ESC : public Pager
{
    public:
    int hand;
    int RESET_RANGE = 50;
    int last_reset;
    ESC(int num_frames);
    frame_t* select_victim_frame();
};

class Aging : public Pager
{
    public:
    //unsigned long age;
    int hand;
    Aging(int num_frames);
    frame_t* select_victim_frame();
    void reset_counter(frame_t* frame, unsigned long age);  // here counter is age as defined in aging
};

class WSet : public Pager
{
    public:
    int hand;
    unsigned TAU = 49;
    WSet(int num_frames);
    void reset_counter(frame_t* frame, unsigned long age);  // here counter = last use time
    frame_t* select_victim_frame();
};

#endif