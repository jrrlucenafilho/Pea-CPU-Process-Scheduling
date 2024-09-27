#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <queue>
#include <algorithm>
#include <iomanip>

#define QUANTUM 2

using namespace std;

typedef struct {
    int arrival_time;
    int execution_duration; // Peak time

    // Only really used for RR. Only one i really had to store these in the type itself
    int completion_time;
    int return_time;
    int wait_time;
    int answer_time;
    int exec_start_time; // Effective start, not arrival
} process_t;

typedef struct {
    float avg_return_time;
    float avg_answer_time;
    float avg_wait_time;
} avg_times_t;

vector<process_t> read_instance(string& filepath)
{
    vector<process_t> proc_vec;
    process_t proc;
    ifstream fp(filepath);

    if(!fp.is_open()){
        cerr << "Error opening file!\n";
        return proc_vec;
    }

    while(fp >> proc.arrival_time >> proc.execution_duration){
        proc_vec.push_back(proc);
    }

    fp.close();
    return proc_vec;
}

// Scheduling Algorithms
avg_times_t first_come_first_serve(vector<process_t>& proc_vec)
{
    avg_times_t avg_times;
    int answer_time_sum = 0;
    int return_time_sum = 0;
    int wait_time_sum = 0;
    int elapsed_time = 0;   // Actually is the time right as current process begins executing

    // Summing up answer times ----------
    for(int i = 0; i < (int)proc_vec.size(); i++){
        // During first proc elapsed time should be zero, still
        if(i == 0){
            answer_time_sum = 0;    // Answer time is zero for first proc on FCFS w/ no preemption
            elapsed_time += proc_vec[i].execution_duration; // But time still passes
            continue;
        }

        // Now for 2nd proc onwards
        // Welp, except when there's a gap in the cpu time
        if((elapsed_time >= proc_vec[i].arrival_time) || (elapsed_time == 0)){
            // If no gap (if gap, simply sum zero to it (do nothing))
            answer_time_sum += elapsed_time - proc_vec[i].arrival_time; // time_since_proc_started - proc's_arrival_time
        }else{
            //In case of a gap, it's good to resync elapsed_time to reality (account for the time gap)
            elapsed_time += proc_vec[i].arrival_time - elapsed_time;
        }
    
        // And time still passes 'till procs finish executing
        elapsed_time += proc_vec[i].execution_duration;
    }

    avg_times.avg_answer_time = answer_time_sum / (float)proc_vec.size();

    // Summing up return times ----------
    elapsed_time = 0;

    for(int i = 0; i < (int)proc_vec.size(); i++){
        // Corrects time gaps
        if((elapsed_time >= proc_vec[i].arrival_time) || (elapsed_time == 0)){
            return_time_sum += (elapsed_time + proc_vec[i].execution_duration) - proc_vec[i].arrival_time;
        }else{
            return_time_sum += proc_vec[i].execution_duration;
            elapsed_time += proc_vec[i].arrival_time - elapsed_time;
        }
        elapsed_time += proc_vec[i].execution_duration;
    }

    avg_times.avg_return_time = return_time_sum / (float)proc_vec.size();

    // Summing up wait times ----------
    elapsed_time = 0;

    for(int i = 0; i < (int)proc_vec.size(); i++){
        // Corrects time gaps
        if((elapsed_time >= proc_vec[i].arrival_time) || (elapsed_time == 0)){
            wait_time_sum += elapsed_time - proc_vec[i].arrival_time;
        }else{
            elapsed_time += proc_vec[i].arrival_time - elapsed_time;
        }

        elapsed_time += proc_vec[i].execution_duration;
    }

    avg_times.avg_wait_time = wait_time_sum / (float)proc_vec.size();

    return avg_times;
}

avg_times_t shortest_job_first(vector<process_t>& proc_vec)
{
    avg_times_t avg_times;
    vector<process_t> rearr_proc_vec = proc_vec;
    int elapsed_time = 0;

    // Rearrange vec to fit it to sjf
    // Also don't want to mess with the original vec
    for(int i = 0; i < (int)rearr_proc_vec.size(); i++){
        for(int j = i + 1; j < (int)rearr_proc_vec.size(); j++){
            // If both are lower than elapsed_time (have arrived), push the one with lower exec_duration to the first's position, might be a bit overkill
            if((rearr_proc_vec[i].arrival_time <= elapsed_time) && (rearr_proc_vec[j].arrival_time <= elapsed_time) && (rearr_proc_vec[i].execution_duration < rearr_proc_vec[j].execution_duration)){
                swap(rearr_proc_vec[i], rearr_proc_vec[j]);
            }
            if((rearr_proc_vec[i].arrival_time <= elapsed_time) && (rearr_proc_vec[j].arrival_time <= elapsed_time) && (rearr_proc_vec[i].execution_duration > rearr_proc_vec[j].execution_duration)){
                swap(rearr_proc_vec[j], rearr_proc_vec[i]);
            }
        }
        elapsed_time += rearr_proc_vec[i].execution_duration;
    }

    avg_times = first_come_first_serve(rearr_proc_vec);

    return avg_times;
}

avg_times_t round_robin(vector<process_t>& proc_vec)
{
    avg_times_t avg_times;
    // Holds to-be-executed procs
    // Apply quantum decrease to proc and pop it
    queue<int> procs_index_queue; // Might be able to be just a var
    vector<bool> procs_in_queue_tracker(proc_vec.size());  // Tracks which procs are curr in queue, pre-sized so it won't run out of bounds
    vector<int> remaining_exec_time_tracker(proc_vec.size()); // Keeps track of all procs exec times through time
    int elapsed_time = 0;
    int finished_procs = 0;
    int index;  // The index (or pid) of current iter's proc
    int process_quant = proc_vec.size();
    float sum_aswer_times = 0;
    float sum_return_times = 0;
    float sum_wait_times = 0;

    procs_index_queue.push(0);
    procs_in_queue_tracker[0] = true; // There'll be at least one proc in there, first one begins (minor correction since i've reserved space for the vec'', should work the same)

    // Everyone begins at their full exec times
    for(int i = 0; i < process_quant; i++){
        remaining_exec_time_tracker[i] = proc_vec[i].execution_duration;
    }

    // First sort them by arrival time
    sort(proc_vec.begin(), proc_vec.end(), [](const process_t& a, const process_t& b){return a.arrival_time < b.arrival_time;});

    while(finished_procs != process_quant){
        index = procs_index_queue.front();
        procs_index_queue.pop();

        // Compares current proc's exec_time with it's current_time at the moment, being track by the array
        // If the remaining is equal to the total exec_time, it's the first time this proc's exec'ing
        if(remaining_exec_time_tracker[index] == proc_vec[index].execution_duration){
            proc_vec[index].exec_start_time = max(elapsed_time, proc_vec[index].arrival_time);
            elapsed_time = proc_vec[index].exec_start_time;
        }

        // Is-proc-finished-executing check, decrease it if not
        if(remaining_exec_time_tracker[index] - QUANTUM > 0){
            remaining_exec_time_tracker[index] -= QUANTUM;
            elapsed_time += QUANTUM;
        }else{
            // Case of the proc actually having finished
            elapsed_time += remaining_exec_time_tracker[index]; //Actually do the changes in the ds' and add to finished_proc counter
            remaining_exec_time_tracker[index] = 0;
            finished_procs++;

            // Deal with stored times
            proc_vec[index].completion_time = elapsed_time;
            proc_vec[index].return_time = proc_vec[index].completion_time - proc_vec[index].arrival_time;
            proc_vec[index].wait_time = proc_vec[index].return_time - proc_vec[index].execution_duration;
            proc_vec[index].answer_time = proc_vec[index].exec_start_time - proc_vec[index].arrival_time;

            // Summing up times, not actual averages yet. Just using to them to hold these values for now
            sum_return_times += proc_vec[index].return_time;
            sum_wait_times += proc_vec[index].wait_time;
            sum_aswer_times += proc_vec[index].answer_time;
        }

        // If there's still time left in each proc, and it HAS arrived (procs_in_queue_tracker false check might be redundant. Being safe). Push it into queue
        // Aside from current proc duh
        for(int i = 1; i < process_quant; i++){
            if((remaining_exec_time_tracker[i] > 0) && (proc_vec[i].arrival_time <= elapsed_time) && (procs_in_queue_tracker[i] == false)){
                procs_index_queue.push(i);
                procs_in_queue_tracker[i] = true;
            }
        }

        // If there's still time left, push curr proc to queue's end
        if(remaining_exec_time_tracker[index] > 0){
            procs_index_queue.push(index);
        }

        // When queue empty, check to be sure that all proc's remaining_time has reached zero
        // Pushing back the first one it finds non-finished
        if(procs_index_queue.empty()){
            for(int i = 1; i < process_quant; i++){
                if(remaining_exec_time_tracker[i] > 0){
                    procs_index_queue.push(i);
                    procs_in_queue_tracker[i] = true;
                    break;
                }
            }
        }
    }

    avg_times.avg_answer_time = sum_aswer_times / process_quant;
    avg_times.avg_return_time = sum_return_times / process_quant;
    avg_times.avg_wait_time = sum_wait_times / process_quant;
    
    return avg_times;
}

int main(void)
{
    vector<process_t> proc_vec;
    string filepath;
    avg_times_t avg_times;

    cin >> filepath;
    proc_vec = read_instance(filepath);

    // In case it couldn't read file
    if(proc_vec.empty()){
        return 1;
    }

    cout << fixed << setprecision(1);

    avg_times = first_come_first_serve(proc_vec);
    cout << "FCFS " << avg_times.avg_return_time << ' ' << avg_times.avg_answer_time << ' ' << avg_times.avg_wait_time << '\n';

    avg_times = shortest_job_first(proc_vec);
    cout << "SJF " << avg_times.avg_return_time << ' ' << avg_times.avg_answer_time << ' ' << avg_times.avg_wait_time << '\n';

    avg_times = round_robin(proc_vec);
    cout << "RR " << avg_times.avg_return_time << ' ' << avg_times.avg_answer_time << ' ' << avg_times.avg_wait_time << '\n';

    return 0;
}