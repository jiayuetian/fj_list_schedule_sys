/**
 * This head file defines some data structures needed in scheduling system.
 * Mainly define task set structure and core structure.
 * 
 * [Scheduling mechanism]:
 * - Fork-join vs. List Scheduling
 * [Data Set]:
 * - To serve both scheduling mechanism, I use DAG task structure.
 * - Task will be generated with essential parameters for two different scheduling ways
 * - For Fork-join: a task is splited into several segments and holds a segment array.
 *                  For each segment of each task, it holds all subtasks which will be executed in segment. 
 * 
 * - For List scheduling: treat a subtask as node, and define type of node, next node, previous nodes number.
 *                        For each node, it only has one next node, but can have more than one previous nodes.
 * 
 * 
 * [Core]:
 * - Be used for calibrate mode, to find best loop number of subtask function.
**/


#ifndef HEADER_H
#define HEADER_H


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h> 
#include <linux/time.h>
#include <linux/types.h>


#define MILLION_SEC 1000000

#define CALIBRATE 0
#define SCHEDULER 1
#define RUN 2

#define TASK_NUM 4
#define CORE_NUM 12
#define SUBTASK_NUM 27
#define RECUR_TIME 4

#define MAX_SUBTASK_NUM 4
#define MAX_SEGMENT_NUM 5
#define MAX_SUBTASK_SUM TASK_SUM*MAX_SEGMENT_NUM*MAX_SUBTASK_NUM

#define PERIOD 200
#define LOOP_COUNT 5300

#define HIGH_PRIORITY 40
#define LOW_PRIORITY 20
#define UTILIZATION 20

struct subtask{
    int no;
    int subtask_no;/*subtask number*/
    int parent_task_no;
    int index_in_parent; /*subtask index in parent task's subtask array*/
    struct task *parent; /*pointer to parent task*/

    int priority;
    //struct hrtimer hr_timer;
    int loop_count;

    int release_time; /*the barrier, the start time of the segment which subtask belongs to*/
    int exec_time; /*the real running time length for current subtask*/
    int offset;

    /*same to segment period, hold by segment*/
    int relative_ddl; /*the longest time allowed for current subtask*/

    /*hold by segment*/
    int cumulative_ddl;/* cumulative_ddl == release_time + relative_ddl*/
    
    int core_no;/*the core which the subtask use*/
    int index_in_core; /*subtask index in parent core's subtask array*/
    int utilization; /*task->exec_time / task->period*/

    int heavy_flag;

    //For list scheduling:
    int next_node;
    int prev_num;
    int node_type;
    int path;

    //indicate if this subtask is assigned or not.
    int assigned_flag;  
};
typedef struct subtask Subtask;
 

struct task{
    int task_no; /*task number*/

    int task_period;
    int priority;
    int curr_seg[CORE_NUM];
    int worst_exec_time;
    int critical_path;

    ktime_t release_time;
    int cumulative_ddl;

    int subtask_sum_in_task;
    int exec_time; /*simply sum up the execution times of all of its subtasks*/
    int util;
    int offset;
    Subtask* subtask_arr[SUBTASK_NUM];
    
    struct hrtimer task_timer;

    int start_core_no;
    int end_core_no;
    int core_num;
    //high utilization task or not 1: high; 0: low
    //high task: list scheduling;
    //low task: EDF(earliest deadline first, computed based on execution time)
    int high_flag;

    //int start_subtask;
    //int end_subtask;
};
typedef struct task Task;

struct core{
    int core_no; /*cpu core number*/
    ktime_t curr_release_time;
    int core_util;
    int last_release;
    int subtask_sum_in_core; /* total number of subtask*/
    int finish_num;
    struct hrtimer timer_arr;
    Subtask* subtask_arr_in_core[SUBTASK_NUM]; /*store all subtasks working with this core*/
};
typedef struct core Core;


struct sched_param{
	int sched_priority;
};
struct sched_param param;


#endif














