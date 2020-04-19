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
#include <linux/mutex.h>



#define MILLION_SEC 1000000

#define CALIBRATE 0
#define SCHEDULER 1
#define RUN 2

#define TASK_NUM 4
#define CORE_NUM 12
#define SUBTASK_NUM 32
#define FINISH_BOUND TASK_NUM*CORE_NUM

#define MAX_SUBTASK_NUM 4
#define MAX_SEGMENT_NUM 5
#define MAX_SUBTASK_SUM TASK_SUM*MAX_SEGMENT_NUM*MAX_SUBTASK_NUM

#define PERIOD 200
#define LOOP_COUNT 5000

#define HIGH_PRIORITY 40
#define LOW_PRIORITY 20
#define UTILIZATION 20
#define RECUR_TIME 1

struct subtask{
    int no;
    int subtask_no;/*subtask number*/
    int parent_task_no;
    int index_in_parent; /*subtask index in parent task's subtask array*/
    struct task *parent; /*pointer to parent task*/

    int segment_no;
    int priority;
    //struct hrtimer hr_timer;
    int loop_count;

    int release_time; /*the barrier, the start time of the segment which subtask belongs to*/
    int exec_time; /*the real running time length for current subtask*/
    int offset;
    /*same to segment period, hold by segment*/
    int relative_ddl; /*the longest time allowed for current subtask*/
    /*hold by segment*/
    int finish_time;/* cumulative_ddl == release_time + relative_ddl*/
    
    int core_no;/*the core which the subtask use*/
    int index_in_core; /*subtask index in parent core's subtask array*/
    int utilization; /*task->exec_time / task->period*/

    int heavy_flag;

    //char* kthread_id;/*may not useful here*/ 
    //struct task_struct *sub_thread; /*may not useful here*/    
};
typedef struct subtask Subtask;
 

struct segment{
    int segment_no; /*No. of segment*/
    int segment_period; /*time period of segement*/
    int subtask_num; /*subtask in this segment each task*/
    int seg_release;
    ktime_t barrier; /*release time*/
    ktime_t cumulative_ddl; /*segment deadline, all subtasks should finish before this time*/
    Subtask *subtask_arr[MAX_SUBTASK_NUM];
    int seg_heavy;
    int seg_priirity;
    int seg_critical_path;
};
typedef struct segment Segment;

struct task{
    int task_no;/*task number*/

    int task_period;
    int priority;
    int segment_num;
    int curr_seg[CORE_NUM];
    int worst_exec_time;
    int critical_path;

    ktime_t release_time;
    int cumulative_ddl;

    int subtask_sum_in_task;
    int exec_time;/*simply sum up the execution times of all of its subtasks*/
    int util;
    int offset;
    struct segment seg_arr[MAX_SEGMENT_NUM];
    
    struct task_struct *thread_arr[CORE_NUM];
    struct hrtimer task_timer;
    int id[CORE_NUM];

    
};
typedef struct task Task;


struct core{
    int core_no; /*cpu core number*/
    ktime_t curr_release_time;
    int core_util;
    int last_release;
    int subtask_sum_in_core; /* total number of subtask*/
    struct hrtimer timer_arr;
    int subtask_arr_in_core[SUBTASK_NUM]; /*store all subtasks working with this core*/
    
};
typedef struct core Core;

struct task_param{
    int core_no;
    int task_no;
};
struct task_param taskinfo[TASK_NUM][CORE_NUM];
struct sched_param{
	int sched_priority;
};
struct sched_param param;


#endif
