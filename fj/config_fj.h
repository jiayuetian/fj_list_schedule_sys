#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h> 
#include <linux/time.h>
#include <linux/types.h>
#include "head_fj.h"



char *mode_str = "schedule";
module_param(mode_str,charp,0644);


char *assign_str = "worst";
module_param(assign_str,charp,0644);


/*test part:task setting*/

/*********************this part is used for task set configuration**********************/


/*period | priority | segment_number | worst_case_exexcution_time |critical_path_length | utilization*/
enum task_feature{Task_Period,Task_Prior, Segment_Num, W_C_Exec_Time, Cri_Path, Util,Task_Offset};
enum task_feature task_fea;

/**********************this part is used for all subtasks configuration**********************/
/*used for scheduler computing*/
/* task_No. | segment_No. | subtask_no | exec_time | relative_deadline | priority | core | real_relaese_time | offset | heavy | */
enum subtask_feature{No,Task_No, Seg_No, Sub_No, Exec_Time, Rela_Ddl, Prio,Core_No, Release_Time, Offset, Heavy_Flag, Next_Node, Node_Type,Prev_Num, Criti_Path};
enum subtask_feature subtask_fea;

int subtask_num_arr[TASK_NUM][MAX_SEGMENT_NUM]={ { 1, 4, 4, 1, 0 },{ 4, 1, 1, 1, 1 },{ 2, 2, 3, 4, 1 },{ 1, 1, 0, 0, 0 }};
int subtask_exec_time_config[TASK_NUM][MAX_SEGMENT_NUM]={ { 10, 25, 25, 60, 0 },{ 25, 30, 30, 30, 60 },{ 25, 20, 23, 30, 50 },{ 170, 180, 0, 0, 0 }};
int task_config[TASK_NUM][7]={ 
{ 1000, 20, 4, 270, 120, 27, 0 }, 
{ 1000, 20, 5, 250, 175, 25, 0 }, 
{ 1000, 20, 5, 330, 148, 33, 0 }, 
{ 1000, 20, 2, 350, 350, 35, 0 } };
int subtask_config_tbl[SUBTASK_NUM][15]={ 
{ 0, 0, 0, 0, 10, -1, -1, -1, -1, 0, -1, 4, -1, 0, -1 }, 
{ 1, 0, 1, 0, 25, -1, -1, -1, -1, 0, -1, 7, -1, 0, -1 }, 
{ 2, 0, 1, 1, 25, -1, -1, -1, -1, 0, -1, 5, -1, 0, -1 }, 
{ 3, 0, 1, 2, 25, -1, -1, -1, -1, 0, -1, 7, -1, 0, -1 }, 
{ 4, 0, 1, 3, 25, -1, -1, -1, -1, 0, -1, 5, -1, 0, -1 }, 
{ 5, 0, 2, 0, 25, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 6, 0, 2, 1, 25, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 7, 0, 2, 2, 25, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 8, 0, 2, 3, 25, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 9, 0, 3, 0, 60, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 10, 1, 0, 0, 25, -1, -1, -1, -1, 0, -1, 16, -1, 0, -1 }, 
{ 11, 1, 0, 1, 25, -1, -1, -1, -1, 0, -1, 16, -1, 0, -1 }, 
{ 12, 1, 0, 2, 25, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 13, 1, 0, 3, 25, -1, -1, -1, -1, 0, -1, 16, -1, 0, -1 }, 
{ 14, 1, 1, 0, 30, -1, -1, -1, -1, 0, -1, 16, -1, 0, -1 }, 
{ 15, 1, 2, 0, 30, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 16, 1, 3, 0, 30, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 17, 1, 4, 0, 60, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 18, 2, 0, 0, 25, -1, -1, -1, -1, 0, -1, 22, -1, 0, -1 }, 
{ 19, 2, 0, 1, 25, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 20, 2, 1, 0, 20, -1, -1, -1, -1, 0, -1, 22, -1, 0, -1 }, 
{ 21, 2, 1, 1, 20, -1, -1, -1, -1, 0, -1, 25, -1, 0, -1 }, 
{ 22, 2, 2, 0, 23, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 23, 2, 2, 1, 23, -1, -1, -1, -1, 0, -1, 28, -1, 0, -1 }, 
{ 24, 2, 2, 2, 23, -1, -1, -1, -1, 0, -1, 28, -1, 0, -1 }, 
{ 25, 2, 3, 0, 30, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 26, 2, 3, 1, 30, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 27, 2, 3, 2, 30, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 28, 2, 3, 3, 30, -1, -1, -1, -1, 0, -1, 29, -1, 0, -1 }, 
{ 29, 2, 4, 0, 50, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 30, 3, 0, 0, 170, -1, -1, -1, -1, 0, -1, 31, -1, 0, -1 }, 
{ 31, 3, 1, 0, 180, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 } 
};







Core* core_set;
Task* task_set;
Subtask* subtask_set;
struct task_struct* kthreads[CORE_NUM];
struct hrtimer timers_core[CORE_NUM];
int sorted_subtask_arr[SUBTASK_NUM];
struct hrtimer start_timer;

int run_num;


