#include <iostream>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <list>
#include<algorithm>
#include <vector>

using namespace std;


vector<int> random_values;
int curio = 0;
int total_io_time;
int io_start_time;
int last_event_finish_time;
int random_index = 0;
int total_cpu_usage = 0;

int algo;
int quantum = 10000;
int maxprio = 4;

//transitions from and to states
enum Transitions { TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_COMPLETE};

//process states
enum State {CREATED, READY, RUNNING, COMPLETED, BLOCKED, PREEMPT};

//algorithms
const int FCFSc = 1;
const int LCFSc = 2;
const int SRTFc = 3;
const int RRc = 4;
const int PRIOc = 5;
const int PREPRIOc = 6;


class Process
{
public:
    int pid; //processid
    int AT; //arrival time
    int TC; //total cpu time
    int original_TC; //total cpu time when process just created
    int CB; //cpu burst
    int IO; //io burst
    int remaining_cb; //remaining cpu burst time
    int turn_around_time;
    State process_state;
    int io_time;
    int waiting_time;
    int completion_time;
    int static_prio;
    int dynamic_prio;
};

class Event
{
public:
    int procid; //
    int timestamp;
    State prev_state;
    Transitions trans;
};

vector<Process> all_processes;
vector<vector<Process*>> active;
vector<vector<Process*>> expire;
vector<int> procid_vec;
vector<int> cur_time_vec;

class DES
{
public:
    vector<Event> eventQ;//use list
    Event get_event()
    {
    		Event e_temp;
    		if(eventQ.empty())
    		{
    			e_temp.procid = -1;
    			e_temp.timestamp = -1;
    		}
    		else
    		{
    			e_temp = eventQ.front();
    			eventQ.erase(eventQ.begin());
    		}
    		return e_temp;
    }
    void put_event(Event event)
    {
    		int flag = 0;
         for(int i =0; i<eventQ.size(); i++)
         {
            //if(it->timestamp <= event.timestamp)
            if(eventQ[i].timestamp <= event.timestamp)
            {
                continue;
            }
            eventQ.insert(eventQ.begin() + i,event);
            flag = 1;
            break; //or break;
         }
         if (flag == 1)
         {
         	return;
         }
         else
         {
         	eventQ.push_back(event);
         }
    }
    int get_next_time()
    {
        if (eventQ.empty())
        {
            return -1;
        }
        else
        {
            return eventQ.front().timestamp;
        }
    }
    void delete_event(int processid)
    {
        int flag = 0;
        int i;
        for(i=0;i<eventQ.size(); i++)
        {
            if(eventQ[i].procid == processid)
            {
                flag = 1;
                break;
            }
        }
        if (flag == 1)
        {
            eventQ.erase(eventQ.begin() + i);
        }
    }
};

DES desobj;
vector<Process*> runQ; //use list?

class Scheduler
{
public:
    //functions implemented as given in question
    virtual void add_process(Process* p) = 0;
    virtual Process* get_next_process() = 0;
    virtual void event_preemption(Process* cur_run_proc, Process* ready_proc, int cur_time) { }
};

Scheduler *sobj;

class FCFS:public Scheduler
{
public:
    void add_process(Process* p1)
    {
        runQ.push_back(p1);
    }
    Process* get_next_process()
    {
        if(runQ.empty()) //runQ.empty()??
        {
            // Process p_temp;
            // p_temp.pid = -1;
            // return p_temp;
            return nullptr;
        }
        else
        {
        		Process * p_temp = runQ.front();
         	runQ.erase(runQ.begin());
         	return p_temp;
        }
    }
};

class LCFS:public Scheduler
{
public:
    void add_process(Process * p1)
    {
        // runQ.push_back(p1);
    	  runQ.insert(runQ.begin(),p1);
    }
    Process * get_next_process()
    {
        if(runQ.empty())
        {
            return nullptr;
        }
        else
        {
	        Process * p_temp = runQ.front();
	        runQ.erase(runQ.begin());
	        return p_temp;
        }
    }
};

class SRTF:public Scheduler
{
public:
    void add_process(Process* p1)
    {
        vector<Process*>::iterator it;
        for(it = runQ.begin(); it != runQ.end(); it++)
        {
            if((*it)->TC <= p1->TC)
            {
                continue;
            }
            runQ.insert(it,p1);
            return;
        }
        if(it == runQ.end())
        {
            runQ.push_back(p1);
        }
    }
    Process* get_next_process()
    {
        if(runQ.size() == 0)
        {
            return NULL;
        }
        Process* p_temp;
        p_temp = runQ.front();
        runQ.erase(runQ.begin());
        return p_temp;
    }
};

class RR:public Scheduler
{
public:
    void add_process(Process * p1)
    {
        runQ.push_back(p1);
    }
    Process * get_next_process()
    {
        if(runQ.size() == 0)
        {
            return NULL;
        }
        Process* p_temp;
        p_temp = runQ.front();
        runQ.erase(runQ.begin());
        return p_temp;
    }
};

class PRIO: public Scheduler
{
public:
    void add_process(Process * p1)
    {
        if(!(p1->dynamic_prio < 0))
        {
            active.at(p1->dynamic_prio).push_back(p1);
        }
        else
        {
            p1->dynamic_prio = p1->static_prio - 1;
            expire.at(p1->dynamic_prio).push_back(p1);
        }
    }
    Process* get_next_process()
    {
        bool active_empty=true;
        bool expire_empty=true;
        int priority;
        priority = maxprio - 1;
        for(int i=0;i<maxprio;i++)
        {
            if(active.at(i).size() != 0)
            {
                active_empty = false;
                break;
            }
        }
        for(int i=0;i<maxprio;i++)
        {
            if(expire.at(i).size() != 0)
            {
                expire_empty = false;
                break;
            }
        }
        if(active_empty == true)
        {
            if(expire_empty == false)
            {
                vector<vector<Process*>> temp_vec;
                temp_vec = active;
                active = expire;
                expire = temp_vec;
            }
            else
            {
                return NULL;
            }
        }
        for(int i=priority; i>=0; i--)
        {
            if(active.at(i).size() != 0)
            {
                Process * p_temp;
                p_temp = active.at(i).front();
                active.at(i).erase(active.at(i).begin());
                return p_temp;
            }
        }
        return NULL;
    }
};

class PREPRIO: public Scheduler
{
public:
    void add_process(Process * p1)
    {
        if(!(p1->dynamic_prio < 0))
        {
            active.at(p1->dynamic_prio).push_back(p1);
        }
        else
        {
            p1->dynamic_prio = p1->static_prio - 1;
            expire.at(p1->dynamic_prio).push_back(p1);
        }
    }
    Process* get_next_process()
    {
        bool active_empty=true;
        bool expire_empty=true;
        int priority;
        priority = maxprio - 1;
        for(int i=0;i<maxprio;i++)
        {
            if(active.at(i).size() != 0)
            {
                active_empty = false;
                break;
            }
        }
        for(int i=0;i<maxprio;i++)
        {
            if(expire.at(i).size() != 0)
            {
                expire_empty = false;
                break;
            }
        }
        if(active_empty == true)
        {
            if(expire_empty == false)
            {
                vector<vector<Process*>> temp_vec;
                temp_vec = active;
                active = expire;
                expire = temp_vec;
            }
            else
            {
                return nullptr;
            }
        }
        for(int i=priority; i>=0; i--)
        {
            if(active.at(i).size() != 0)
            {
                Process * p_temp;
                p_temp = active.at(i).front();
                active.at(i).erase(active.at(i).begin());
                return p_temp;
            }
        }
        return nullptr;
    }

    void event_preemption(Process * cur_run_proc, Process* ready_proc, int cur_time)
    {
        int next_time = 0;
        if(cur_run_proc != nullptr)
        {
            cur_time_vec.push_back(cur_time);
            for(vector<Event>::iterator it=desobj.eventQ.begin(); it != desobj.eventQ.end(); it++)
            {
                if((*it).procid == cur_run_proc->pid)
                {
                    next_time = (*it).timestamp;
                    break;
                }
            }
        }
        else
        {
            return;
        }
        if((next_time > cur_time) && (ready_proc->dynamic_prio > cur_run_proc->dynamic_prio))
        {
            desobj.delete_event(cur_run_proc->pid);
            cur_time_vec.push_back(cur_time);
            cur_run_proc->TC += next_time - cur_time;
            cur_run_proc->remaining_cb += next_time - cur_time;
            Event e_temp;
            e_temp.timestamp = cur_time;
            e_temp.procid = cur_run_proc->pid;
            e_temp.prev_state = RUNNING;
            e_temp.trans = TRANS_TO_PREEMPT;
            desobj.put_event(e_temp);
        }
    }
};

int myrandom(int burst)
{
    if(random_index == random_values.size()-1)
    {
        random_index = 0;
    }
    random_index += 1;
    return 1 + (random_values[random_index] % burst);
}

void simulation()
{
    Process * current_process;
    //use pointer becasue sobj pointer
    // and virtual func get_next_process return pointer
    current_process = nullptr;

    bool call_scheduler = false;

    //Event* e;
    Event e;
    int current_time = 0;
    int time_in_prev_state;
    int burst_time; //put this inside while

    while(true)
    {
        e = desobj.get_event();
        if(e.procid != -1)
        {
            int p = e.procid - 1; //process to work on
            current_time = e.timestamp;

            switch(e.trans)
            {
                case TRANS_TO_READY:
                {
                    //cout<<"in trans to ready"<<endl;
                    all_processes[p].process_state = READY;
                    if(e.prev_state == BLOCKED)
                    {
                        curio -= 1;
                        if(curio == 0)
                        {
                            total_io_time += current_time - io_start_time;
                        }
                    }
                    if(algo != FCFSc && algo != LCFSc && algo != SRTFc && algo != RRc && algo != PRIOc)
                    {
                        sobj->event_preemption(current_process, &all_processes[p], current_time);
                    }
                    sobj->add_process(&all_processes[p]);
                    procid_vec.push_back(p);
                    call_scheduler = true;
                    break;
                }
                case TRANS_TO_RUN:
                {
                    all_processes[p].process_state = RUNNING;
                    if (all_processes[p].remaining_cb > 0)
                    {
                        burst_time = all_processes[p].remaining_cb;
                    }
                    else
                    {
                        int random_number;
                        int burst_pass = all_processes[p].CB;
                        random_number = myrandom(burst_pass);
                        int rem_tc = all_processes[p].TC;
                        burst_time = min(random_number, rem_tc);
                    }

                    if (algo == RRc || algo == PRIOc || algo == PREPRIOc)
                    {
                        if (all_processes[p].TC <= burst_time && all_processes[p].TC <= quantum)
                        {
                            all_processes[p].process_state = COMPLETED;
                            Event e_new;
                            e_new.procid = all_processes[p].pid;
                            procid_vec.push_back(p);
                            int rem_tc = all_processes[p].TC;
                            e_new.timestamp = current_time + rem_tc;
                            all_processes[p].TC = 0;
                            all_processes[p].remaining_cb = 0;
                            e_new.prev_state = RUNNING;
                            e_new.trans = TRANS_TO_COMPLETE;
                            desobj.put_event(e_new);
                        }
                        else if (burst_time <= quantum)
                        {
                            all_processes[p].process_state = BLOCKED;
                            procid_vec.push_back(p);
                            all_processes[p].remaining_cb = 0;
                            all_processes[p].TC -= burst_time;
                            Event e_new;
                            e_new.procid = all_processes[p].pid; //
                            e_new.timestamp = current_time + burst_time;
                            e_new.prev_state = RUNNING;
                            e_new.trans = TRANS_TO_BLOCK;
                            desobj.put_event(e_new);
                        }
                        else
                        {
                            all_processes[p].process_state = PREEMPT;
                            procid_vec.push_back(p);
                            all_processes[p].remaining_cb = burst_time - quantum;
                            all_processes[p].TC -= quantum;
                            Event e_new;
                            e_new.procid = all_processes[p].pid;
                            e_new.timestamp = current_time + quantum;
                            e_new.prev_state = RUNNING;
                            e_new.trans = TRANS_TO_PREEMPT;
                            desobj.put_event(e_new);
                        }
                    }
                    else if (all_processes[p].TC - burst_time > 0)
                    {
                        all_processes[p].process_state = BLOCKED;
                        procid_vec.push_back(p);
                        all_processes[p].TC = all_processes[p].TC - burst_time;
                        Event e_new;
                        e_new.procid = all_processes[p].pid; //
                        e_new.timestamp = current_time + burst_time;
                        e_new.prev_state = RUNNING;
                        e_new.trans = TRANS_TO_BLOCK;
                        desobj.put_event(e_new);
                    }
                    else
                    {
                        all_processes[p].process_state = COMPLETED;
                        all_processes[p].TC = 0;
                        procid_vec.push_back(p);
                        Event e_new;
                        e_new.procid = all_processes[p].pid; //
                        e_new.timestamp = current_time + burst_time;
                        e_new.prev_state = RUNNING;
                        e_new.trans = TRANS_TO_COMPLETE;
                        desobj.put_event(e_new);
                    }
                    break;
                }
                case TRANS_TO_BLOCK:
                {
                		//cout<<"in trans to block"<<endl;
                    all_processes[p].process_state = READY;
                    curio += 1;
                    if(curio == 1)
                    {
                        io_start_time = current_time;
                    }
                    int random_number = myrandom(all_processes[p].IO);
                    all_processes[p].io_time += random_number;
                    all_processes[p].dynamic_prio = all_processes[p].static_prio - 1;
                    Event e_temp;
                    e_temp.procid = all_processes[p].pid; //
                    e_temp.timestamp = current_time + random_number;
                    procid_vec.push_back(p);
                    e_temp.prev_state = BLOCKED;
                    e_temp.trans = TRANS_TO_READY;
                    desobj.put_event(e_temp);
                    call_scheduler = true;
                    current_process = nullptr;
                    break;
                }
                case TRANS_TO_PREEMPT:
                {
                    all_processes[p].process_state = READY;
                    procid_vec.push_back(p);
                    all_processes[p].dynamic_prio -= 1;
                    current_process = nullptr;
                    sobj->add_process(&all_processes[p]);
                    call_scheduler = true;
                    break;
                }
                case TRANS_TO_COMPLETE:
                {
                    //cout<<"in trans to complete"<<endl;
                    all_processes[p].completion_time = e.timestamp;
                    procid_vec.push_back(p);
                    all_processes[p].turn_around_time = all_processes[p].completion_time - all_processes[p].AT;
                    all_processes[p].waiting_time = all_processes[p].turn_around_time - all_processes[p].io_time - all_processes[p].original_TC;
                    last_event_finish_time = all_processes[p].completion_time;
                    current_process = nullptr;
                    call_scheduler = true;
                    break;
                }
            }

            //desobj.delete_event(p);
            //cout<<"before schedule call"<<endl;

            if(call_scheduler == true)
            {
                if(desobj.get_next_time() == current_time)
                {
                    continue;
                }
                call_scheduler = false;
                if (current_process == nullptr)
                {
                    current_process = sobj->get_next_process();
                    if (current_process == nullptr)
                    {
                        continue;
                    }
                    Event e_temp;
                    e_temp.procid = current_process->pid;
                    e_temp.timestamp = current_time;
                    e_temp.prev_state = READY;
                    e_temp.trans = TRANS_TO_RUN;
                    desobj.put_event(e_temp);
                }
            }
            //cout<<"end of loop"<<endl;
        }
        else
        {
    		// cout<<"else break";
           break;
        }
    }
}

int main(int argc, char *argv[])
{
    bool flagverbose = false;
    bool flagtrace = false;
    bool flageventq = false;
    bool flagpreemption = false;

    //DES desobj;

    int c;
    char *cvalue;
    int flag1 = 0;

    //checks for arguments implemented as given in doc for getopt
    while ((c = getopt(argc, argv, "vteps:")) != -1)
    {
        switch (c)
        {
            case 'v':
            {
                flagverbose = true;
                break;
            }
            case 's':
            {
                //cvalue = optarg[0];
                switch (optarg[0])
                {
                    case 'F':
                    {
                        algo = FCFSc;
                        sobj = new FCFS();
                        // cout<<algo<<endl;
                        break;
                    }
                    case 'L':
                    {
                        algo = LCFSc;
                        sobj = new LCFS();
                        break;
                    }
                    case 'S':
                    {
                        algo = SRTFc;
                        sobj = new SRTF();
                        break;
                    }
                    case 'R':
                    {
                        optarg++;
                        sscanf(optarg, "%d:%d", &quantum, &maxprio);
                        algo = RRc;
                        sobj = new RR();
                        break;
                    }
                    case 'P':
                    {
                        optarg++;
                        sscanf(optarg, "%d:%d", &quantum, &maxprio);
                        algo = PRIOc;
                        sobj = new PRIO();
                        break;
                    }
                    case 'E':
                    {
                        optarg++;
                        sscanf(optarg, "%d:%d", &quantum, &maxprio);
                        algo = PREPRIOc;
                        sobj = new PREPRIO();
                        break;
                    }
                }
                break;
            }
            default:
            {
                abort();
            }
        }
    }

    active.resize(maxprio);
    expire.resize(maxprio);

    ifstream random_file;
    int randVal, temp;
    random_file.open(argv[optind + 1], ios::in);
    random_file >> randVal;
    random_values.push_back(randVal);
    for (int i = 0; i < randVal; i++)
    {
        random_file >> temp;
        random_values.push_back(temp);
    }
    random_file.close();

    int counter = 1;
    ifstream input_file;
    input_file.open(argv[optind], ios::in);
    string w1,w2,w3,w4;
    while(input_file>>w1>>w2>>w3>>w4)
    {
        int AT1 = stoi(w1);
        int TC1 = stoi(w2);
        int CB1 = stoi(w3);
        int IO1 = stoi(w4);

        total_cpu_usage = total_cpu_usage + TC1;
        Process p1;
        p1.pid = counter;
        p1.AT = AT1;
        p1.TC = TC1;
        p1.original_TC = TC1;
        p1.CB = CB1;
        p1.IO = IO1;
        p1.process_state = READY;
        p1.io_time = 0;
        p1.remaining_cb = 0;
        p1.static_prio = myrandom(maxprio);
        p1.dynamic_prio = p1.static_prio - 1;
        all_processes.push_back(p1);
        ///////////////////////////

        Event e1;
        e1.procid = counter;
        e1.timestamp = AT1;
        e1.prev_state = CREATED;
        e1.trans = TRANS_TO_READY;
        desobj.put_event(e1);

        counter = counter + 1;
    }
    input_file.close();

    simulation();

    if (algo == FCFSc)
    {
        cout<<"FCFS"<<endl;
    }
    else if(algo == LCFSc)
    {
        cout<<"LCFS"<<endl;
    }
    else if(algo == SRTFc)
    {
        cout<<"SRTF"<<endl;
    }
    else if(algo == RRc)
    {
        cout<<"RR"<<" "<<quantum<<endl;
    }
    else if(algo == PRIOc)
    {
        cout<<"PRIO"<<" "<<quantum<<endl;
    }
    else if(algo == PREPRIOc)
    {
        cout<<"PREPRIO"<<" "<<quantum<<endl;
    }

    int average_turn_around = 0;
    int average_waiting_time = 0;

    for(int a = 0; a<all_processes.size();a++)
    {
        average_waiting_time += all_processes[a].waiting_time;
        average_turn_around += all_processes[a].turn_around_time;
        //cout<<all_processes[a].pid<<": "<<all_processes[a].AT<<" "<<all_processes[a].original_TC<<" "<<all_processes[a].CB<<" "<<all_processes[a].IO<<" "<<all_processes[a].static_prio;
        //cout<<" | "<<all_processes[a].completion_time<<" "<<all_processes[a].turn_around_time<<" "<<all_processes[a].io_time<<" "<<all_processes[a].waiting_time<<endl;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",all_processes[a].pid-1, all_processes[a].AT, all_processes[a].original_TC, all_processes[a].CB, all_processes[a].IO, all_processes[a].static_prio, all_processes[a].completion_time, all_processes[a].turn_around_time, all_processes[a].io_time, all_processes[a].waiting_time);
    }

    // last_event_finish_time2 = (double)last_event_finish_time;
    double total_cpu_usage2 = (total_cpu_usage*100)/(double)last_event_finish_time;
    double total_io_time2 = (total_io_time*100)/(double)last_event_finish_time;
    int process_count = counter - 1;
    double average_turn_around2 = average_turn_around/(double)process_count;
    double average_waiting_time2 = average_waiting_time/(double)process_count;
    //cout<<"SUM: "<<last_event_finish_time<<" "<<total_cpu_usage2<<" "<<total_io_time<<" "<<average_turn_around2<<" "<<average_waiting_time2<<" "<<(process_count*100)/(double)last_event_finish_time<<endl;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", last_event_finish_time, total_cpu_usage2, total_io_time2, average_turn_around2, average_waiting_time2, (double)(process_count*100)/(double)last_event_finish_time);

    return 0;
}