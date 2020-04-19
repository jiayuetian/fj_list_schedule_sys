#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h> 
#include <linux/time.h>
#include <linux/types.h>
#include "head_list.h"



char *mode_str = "schedule";
module_param(mode_str,charp,0644);
char *pri_str = "path";
module_param(pri_str,charp,0644);

/*test part:task setting*/

/*********************this part is used for task set configuration**********************/

/*bound of 5/20%*/
/*period | priority | segment_number | worst_case_exexcution_time |critical_path_length | utilization*/
enum task_feature{Task_Period,Task_Prior, Segment_Num, W_C_Exec_Time, Cri_Path, Util,Task_Offset};
enum task_feature task_fea;

/**********************this part is used for all subtasks configuration**********************/
/*used for scheduler computing*/
/* task_No. | segment_No. | subtask_no | exec_time | relative_deadline | priority | core | real_relaese_time | offset | heavy | next_node | node_type | prev_num | criti_path*/
enum subtask_feature{No, Task_No, Seg_No, Sub_No, Exec_Time, Rela_Ddl, Prio,Core_No, Release_Time, Offset, Heavy_Flag, Next_Node, Node_Type,Prev_Num, Criti_Path};
enum subtask_feature subtask_fea;

int subtask_num_arr[TASK_NUM][MAX_SEGMENT_NUM]={ { 3, 2, 4, 1, 1 },{ 2, 1, 0, 0, 0 },{ 3, 1, 0, 0, 0 },{ 3, 2, 3, 1, 0 }};
int subtask_exec_time_config[TASK_NUM][MAX_SEGMENT_NUM]={ { 14, 18, 19, 6, 24 },{ 63, 54, 0, 0, 0 },{ 40, 42, 0, 0, 0 },{ 24, 21, 20, 18, 0 }};
int task_config[TASK_NUM][7]={ 
{ 600, 20, 5, 186, 81, 31, 0 }, 
{ 600, 20, 2, 180, 117, 30, 0 }, 
{ 600, 20, 2, 162, 82, 27, 0 }, 
{ 600, 20, 4, 192, 83, 32, 0 } };
int subtask_config_tbl[SUBTASK_NUM][15]={ 
{ 0, 0, 0, 0, 14, -1, -1, -1, -1, 0, -1, 4, -1, 0, -1 }, 
{ 1, 0, 0, 1, 14, -1, -1, -1, -1, 0, -1, 3, -1, 0, -1 }, 
{ 2, 0, 0, 2, 14, -1, -1, -1, -1, 0, -1, 5, -1, 0, -1 }, 
{ 3, 0, 1, 0, 18, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 4, 0, 1, 1, 18, -1, -1, -1, -1, 0, -1, 10, -1, 0, -1 }, 
{ 5, 0, 2, 0, 19, -1, -1, -1, -1, 0, -1, 10, -1, 0, -1 }, 
{ 6, 0, 2, 1, 19, -1, -1, -1, -1, 0, -1, 10, -1, 0, -1 }, 
{ 7, 0, 2, 2, 19, -1, -1, -1, -1, 0, -1, 10, -1, 0, -1 }, 
{ 8, 0, 2, 3, 19, -1, -1, -1, -1, 0, -1, 9, -1, 0, -1 }, 
{ 9, 0, 3, 0, 6, -1, -1, -1, -1, 0, -1, 10, -1, 0, -1 }, 
{ 10, 0, 4, 0, 24, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 11, 1, 0, 0, 63, -1, -1, -1, -1, 0, -1, 13, -1, 0, -1 }, 
{ 12, 1, 0, 1, 63, -1, -1, -1, -1, 0, -1, 13, -1, 0, -1 }, 
{ 13, 1, 1, 0, 54, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 14, 2, 0, 0, 40, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 15, 2, 0, 1, 40, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 16, 2, 0, 2, 40, -1, -1, -1, -1, 0, -1, 17, -1, 0, -1 }, 
{ 17, 2, 1, 0, 42, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 }, 
{ 18, 3, 0, 0, 24, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 19, 3, 0, 1, 24, -1, -1, -1, -1, 0, -1, 23, -1, 0, -1 }, 
{ 20, 3, 0, 2, 24, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 21, 3, 1, 0, 21, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 22, 3, 1, 1, 21, -1, -1, -1, -1, 0, -1, 24, -1, 0, -1 }, 
{ 23, 3, 2, 0, 20, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 24, 3, 2, 1, 20, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 25, 3, 2, 2, 20, -1, -1, -1, -1, 0, -1, 26, -1, 0, -1 }, 
{ 26, 3, 3, 0, 18, -1, -1, -1, -1, 0, -1, -1, -1, 0, -1 } 
};






















Core* core_set;
Task* task_set;
Subtask* subtask_set;
struct task_struct* kthreads[CORE_NUM];
struct hrtimer timers_core[CORE_NUM];
int sorted_subtask_arr[SUBTASK_NUM];



