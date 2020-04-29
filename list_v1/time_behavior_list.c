#include "head_list.h"
#include "config_list.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h> 
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/types.h>

int run_num = 0;
int finished = 0;

static void init(void) {
    //create task set
    task_set = (Task *)kmalloc_array(TASK_NUM, sizeof(struct task), GFP_KERNEL);
    if (task_set==NULL) {
        printk(KERN_INFO "task kmalloc_array error");
        return -1;
    }
    // create core set
    core_set = (Core*)kmalloc_array(CORE_NUM,sizeof(struct core), GFP_KERNEL);
    if (core_set==NULL) {
        printk(KERN_INFO "core kmalloc_array error");
        return -1;
    }
    
    subtask_set = (Subtask *)kmalloc_array(SUBTASK_NUM, sizeof(struct subtask), GFP_KERNEL); 
    if (subtask_set==NULL) {
        printk(KERN_INFO "subtask kmalloc_array error");
        return -1;
    }
    
    int i = 0;
    int j = 0;
    int k = 0;
    int temp = 0;
    int weight = 0;

    //initialize subtask_set;
    for(i = 0; i < SUBTASK_NUM;i++){
        subtask_set[i].no = i;
        subtask_set[i].parent_task_no = subtask_config_tbl[i][Task_No];
        subtask_set[i].subtask_no = subtask_config_tbl[i][Sub_No];
	subtask_set[i].relative_ddl = subtask_config_tbl[i][Rela_Ddl];
        subtask_set[i].exec_time = subtask_config_tbl[i][Exec_Time];
        subtask_set[i].priority = subtask_config_tbl[i][Prio];
        subtask_set[i].core_no = subtask_config_tbl[i][Core_No];
        subtask_set[i].release_time = subtask_config_tbl[i][Release_Time];
        subtask_set[i].heavy_flag = subtask_config_tbl[i][Heavy_Flag];
        subtask_set[i].next_node = subtask_config_tbl[i][Next_Node];
        subtask_set[i].node_type = subtask_config_tbl[i][Node_Type];
        subtask_set[i].prev_num = subtask_config_tbl[i][Prev_Num];;
        subtask_set[i].path = subtask_config_tbl[i][Criti_Path];;
        subtask_set[i].loop_count = subtask_set[i].exec_time * LOOP_COUNT;
        subtask_set[i].priority = -1;
        subtask_set[i].assigned_flag = 0;
        sorted_subtask_arr[i] = i;
       // printk(KERN_INFO "subtask[%d]:task_no[%d],segment_no[%d],subtask_no[%d],exec_time[%d],priority[%d],core_no[%d]",i,subtask_set[i].parent_task_no,
       //	    subtask_set[i].segment_no,subtask_set[i].subtask_no,subtask_set[i].exec_time,subtask_set[i].priority,subtask_set[i].core_no);
    }
    
    //initialize core set;
    for(i = 0; i < CORE_NUM; i++){
        core_set[i].core_no = i;
        core_set[i].core_util = 0;
        core_set[i].subtask_sum_in_core = 0;
        core_set[i].last_release = 0;
        core_set[i].finish_num = 0;
    }

    int core_count = 0;
    //initialize task_set;
    for(i = 0; i < TASK_NUM; i++){
	task_set[i].task_no = i;
        task_set[i].task_period = task_config[i][Task_Period];
        task_set[i].priority = task_config[i][Task_Prior];
        task_set[i].worst_exec_time = task_config[i][W_C_Exec_Time];
        task_set[i].critical_path = task_config[i][Cri_Path];
        task_set[i].util = task_config[i][Util];
	task_set[i].offset = task_config[i][Task_Offset];
        task_set[i].subtask_sum_in_task = 0;
        task_set[i].start_core_no = -1;
        task_set[i].end_core_no = -1;
        if(task_set[i].util >= 100) {
            task_set[i].high_flag = 1;
            task_set[i].core_num = (task_set[i].worst_exec_time - task_set[i].critical_path) / (task_set[i].task_period - task_set[i].critical_path);
            if (task_set[i].core_num  == 1) task_set[i].core_num = 2;
        }
        else {
            task_set[i].high_flag = 0;
            task_set[i].core_num = 0;
        }
        core_count += task_set[i].core_num;

//	    printk(KERN_INFO "task_no[%d],segment_no[%d],segment_period[%d],seg_critical_path[%d],subtask_sum_in_task[%d],heavy[%d]",task_set[i].task_no,task_set[i].seg_arr[j].segment_no,
//                   task_set[i].seg_arr[j].segment_period,task_set[i].seg_arr[j].seg_critical_path,task_set[i].subtask_sum_in_task,task_set[i].seg_arr[j].seg_heavy);
    }
    if(core_count >= CORE_NUM){
        printk(KERN_INFO "the task set is not schedulable");
    }

    for(i=0;i< SUBTASK_NUM;i++){
        task_set[subtask_set[i].parent_task_no].subtask_arr[task_set[subtask_set[i].parent_task_no].subtask_sum_in_task] = &(subtask_set[i]);
        task_set[subtask_set[i].parent_task_no].subtask_sum_in_task++;
    }
    //for(i=0;i< TASK_NUM;i++){
	//	printk(KERN_INFO "task_no[%d]: high_flag[%d],core_num[%d]",i,task_set[i].high_flag,task_set[i].core_num);
    //}
}

static void prev_node_num_computing(void){
    int i = 0;
    int next = -1;
    for( i = 0 ; i < SUBTASK_NUM; i++){
        next = subtask_set[i].next_node;
        if(next != -1){
            subtask_set[next].prev_num++;
        }
    }
}


static void path_computing(void){
    int i, j = 0;
    for(i = 0; i< SUBTASK_NUM;i++){
        subtask_set[i].path = subtask_set[i].exec_time;
        j = i;
        while(subtask_set[j].next_node != -1){
            j = subtask_set[j].next_node;
            subtask_set[i].path += subtask_set[j].exec_time;
        }
    }
}

static void path_priority_computing(void){
    int i,j = 0;
    int temp_priority = 35;
    int temp_path = 0;
    int find_subtask = -1;
    int temp = 0;
    for(i = 0; i < SUBTASK_NUM; i++){
        temp_path = 0;
        find_subtask = -1;
        for(j = 0; j < SUBTASK_NUM; j++){
            if(subtask_set[j].priority != -1){
                continue;
            }
            if(subtask_set[j].path > temp_path){
                find_subtask = j;
                temp_path = subtask_set[j].path;
            }
        }
        if(find_subtask == -1) break;
        printk(KERN_INFO "find_subtask[%d], path[%d]",find_subtask,temp_path);
        subtask_set[find_subtask].priority = temp_priority;
        sorted_subtask_arr[temp] = find_subtask;
        temp++;

        for(j = 0; j < SUBTASK_NUM; j++){
            if(subtask_set[j].path == temp_path && subtask_set[j].priority == -1){
                subtask_set[j].priority = temp_priority;
                sorted_subtask_arr[temp] = j;
                temp++;
            }
        }
        temp_priority--;
    }
    for( i = 0; i < SUBTASK_NUM; i++){
        printk(KERN_INFO "subtask[%d]: path[%d], priority[%d]",subtask_set[i].no, subtask_set[i].path,subtask_set[i].priority);
    }
}


static void exec_priority_computing(void){
    int i,j = 0;
    int temp_priority = 40;
    int temp_time = 0;
    int find_subtask = -1;
    int temp = 0;
    for(i = 0; i < SUBTASK_NUM; i++){
        temp_time = 0;
        find_subtask = -1;
        for(j = 0; j < SUBTASK_NUM; j++){
            if(subtask_set[j].priority != -1){
                continue;
            }
            if(subtask_set[j].exec_time > temp_time){
                find_subtask = j;
                temp_time = subtask_set[j].exec_time;
            }
        }
        if(find_subtask == -1) break;
        //printk(KERN_INFO "find_subtask[%d], temp_time[%d]",find_subtask,temp_time);
        subtask_set[find_subtask].priority = temp_priority;
        sorted_subtask_arr[temp] = find_subtask;
        temp++;

        for(j = 0; j < SUBTASK_NUM; j++){
            if(subtask_set[j].exec_time == temp_time && subtask_set[j].priority == -1){
                subtask_set[j].priority = temp_priority;
                sorted_subtask_arr[temp] = j;
                temp++;
            }
        }
        temp_priority--;
    }
    for( i = 0; i < SUBTASK_NUM; i++){
        printk(KERN_INFO "subtask[%d]: path[%d], priority[%d] exec[%d]",subtask_set[i].no, subtask_set[i].path,subtask_set[i].priority, subtask_set[i].exec_time);
    }
}


static void sort_priority_in_task(void){
    int i ,j, k = 0;
    Subtask *temp;
    for(i = 0; i< TASK_NUM;i++){
        for(j = 0; j < task_set[i].subtask_sum_in_task; j++){
            temp = task_set[i].subtask_arr[j];
            for(k = j+1; k < task_set[i].subtask_sum_in_task; k++){
                if(task_set[i].subtask_arr[k]->priority > task_set[i].subtask_arr[j]->priority){
                    temp = task_set[i].subtask_arr[j];
                    task_set[i].subtask_arr[j] = task_set[i].subtask_arr[k];
                    task_set[i].subtask_arr[k] = temp;
                }
            }
        }
    }

    //for(i = 0; i< TASK_NUM;i++){
      //  for(j = 0; j < task_set[i].subtask_sum_in_task; j++){
   	//    printk(KERN_INFO "task[%d]:subtask[%d]: path[%d] exec[%d],priority[%d],prev_num[%d]",i,task_set[i].subtask_arr[j]->subtask_no,task_set[i].subtask_arr[j]->path,task_set[i].subtask_arr[j]->exec_time,task_set[i].subtask_arr[j]->priority,task_set[i].subtask_arr[j]->prev_num);
        //}
    //}
}


static void core_assign(void){

    int start = 0;
    int end = 0;
    int core_num = 0;
    int i,j =0;
    int high_task = 0;

    for(i < 0; i< TASK_NUM;i++){
        if(task_set[i].high_flag == 1){
            high_task++;
            task_set[i].start_core_no = start;
            task_set[i].end_core_no = task_set[i].start_core_no + task_set[i].core_num ;
            if(task_set[i].end_core_no > CORE_NUM ) {
                printk(KERN_INFO "This is not schedulable");
                return -1;
            }
            start = task_set[i].end_core_no;
        }
    }
    if(high_task == TASK_NUM && start >= CORE_NUM){
        printk(KERN_INFO "This is not schedulable");
        return -1;
    }

    for(i = 0; i < TASK_NUM; i++){
        if(task_set[i].high_flag == 0){
            task_set[i].start_core_no = start;
            task_set[i].end_core_no = CORE_NUM;
        }
    }
    //   for(i=0;i< TASK_NUM;i++){
	//	printk(KERN_INFO "task_no[%d]: high_flag[%d],core_num[%d],start_core_no[%d], end_core_no[%d]",i,task_set[i].high_flag,task_set[i].core_num,task_set[i].start_core_no,task_set[i].end_core_no);
    //}

}


static void high_task_scheduling(void){
    int i,j,k= 0;
    int start = 0;
    int end = 0;
    int core_num =0;
    int temp = 0;
    int finished = 0;
    int subtask_num = 0;
    int earliest = 0;
    int temp_release = 0;
    int next_node = 0;
    int bound = 0;

    for(i = 0; i < TASK_NUM ; i++){
        //if a task is high utilization task
        if(task_set[i].high_flag == 1){
        
            start = task_set[i].start_core_no;
            end = task_set[i].end_core_no;
            core_num = task_set[i].core_num;
            temp = start;
            subtask_num = task_set[i].subtask_sum_in_task;
            finished = 0;
            bound = 0;
            //printk(KERN_INFO "start[%d], end[%d], core_num[%d], subtask_num[%d]",start,end,core_num,subtask_num);

            //for(k = 0; k < task_set[i].subtask_sum_in_task; k++){
              //  printk(KERN_INFO " task[%d] subtask_arr[%d] prev_num[%d] assigned_flag[%d]",i,k,task_set[i].subtask_arr[k]->prev_num,task_set[i].subtask_arr[k]->assigned_flag);
            //}
            
            earliest = start;
            while(finished < subtask_num && bound < SUBTASK_NUM+10){
                //printk(KERN_INFO "bound[%d] finished[%d] subtask_sum[%d]", bound,finished, task_set[i].subtask_sum_in_task);
                bound++;
                //find the earliest released core;
                // worst-fit
               
                earliest = start + (earliest + 1 - start) % core_num;

                if(core_set[earliest].subtask_sum_in_core > core_set[earliest].finish_num){               
                   next_node = core_set[earliest].subtask_arr_in_core[core_set[earliest].subtask_sum_in_core-1]->next_node;
                   subtask_set[next_node].prev_num-= 1;
                   core_set[earliest].finish_num++;
                }

                //printk(KERN_INFO "find valid core[%d] release time[%d]",earliest,core_set[earliest].last_release);
                //find the subtask which is ready and with highest priority
                
                for(k = 0; k < subtask_num; k++){
                    //printk(KERN_INFO "In loop: task[%d] subtask_arr[%d] prev_num[%d] assigned_flag[%d]",i,k,task_set[i].subtask_arr[k]->prev_num,task_set[i].subtask_arr[k]->assigned_flag);
                    if(task_set[i].subtask_arr[k]->prev_num == 0 && task_set[i].subtask_arr[k]->assigned_flag == 0){
                        //printk(KERN_INFO "core release time[%d] + exec_time[%d]",core_set[earliest].last_release,task_set[i].subtask_arr[k]->exec_time);
                        if(core_set[earliest].last_release + task_set[i].subtask_arr[k]->exec_time < task_set[i].task_period){
                           task_set[i].subtask_arr[k]->assigned_flag = 1;
                           task_set[i].subtask_arr[k]->core_no = earliest;
                           core_set[earliest].last_release += task_set[i].subtask_arr[k]->exec_time;
                           core_set[earliest].subtask_arr_in_core[core_set[earliest].subtask_sum_in_core] = task_set[i].subtask_arr[k];
                           core_set[earliest].subtask_sum_in_core++;
                           finished++;
                           break;
                           
                        }
                    }
            
                }
            
            }

        }

    }
}



static void low_task_scheduling(void){
    int i,j,k = 0;
    int start = 0;
    int end = 0;
    int low_subtask_count = 0;
    int finished = 0;
    int earliest = 0;
    int temp_release = 0;
    int subtask_index = 0;
    int bound = 0;
    int next_node = 0;
    int core_num = 0;

    for(i = 0; i < TASK_NUM;i++){
        if(task_set[i].high_flag == 0){
            low_subtask_count += task_set[i].subtask_sum_in_task;
            start = task_set[i].start_core_no;
            end = task_set[i].end_core_no;
            core_num = end-start;
        }
    }
    //printk(KERN_INFO "total number of low subtasks[%d]", low_subtask_count);


    earliest = start;
    while(finished < low_subtask_count && bound < SUBTASK_NUM+10){
        bound++;
        //worst-fit
        earliest = start + (earliest + 1 - start) % core_num;

        if(core_set[earliest].subtask_sum_in_core > core_set[earliest].finish_num){               
           next_node = core_set[earliest].subtask_arr_in_core[core_set[earliest].subtask_sum_in_core-1]->next_node;
           subtask_set[next_node].prev_num-= 1;
           core_set[earliest].finish_num++;
        }

        for(k = 0; k < SUBTASK_NUM; k++){
            subtask_index = sorted_subtask_arr[k];
            if(task_set[subtask_set[subtask_index].parent_task_no].high_flag == 0 && subtask_set[subtask_index].prev_num == 0 && subtask_set[subtask_index].assigned_flag == 0){
                if(core_set[earliest].last_release + subtask_set[subtask_index].exec_time < task_set[subtask_set[subtask_index].parent_task_no].task_period){
                    subtask_set[subtask_index].assigned_flag=1;
                    subtask_set[subtask_index].core_no = earliest;
                    core_set[earliest].last_release += subtask_set[subtask_index].exec_time;
                    core_set[earliest].subtask_arr_in_core[core_set[earliest].subtask_sum_in_core] = &(subtask_set[subtask_index]);
                    core_set[earliest].subtask_sum_in_core++;
                    finished++;
                    break;
                }
            }
        }

    }
    for(k = 0; k < SUBTASK_NUM; k++){
        printk(KERN_INFO " task[%d] subtask_arr[%d] prev_num[%d] assigned_flag[%d]",subtask_set[k].parent_task_no,subtask_set[k].prev_num,subtask_set[k].prev_num,subtask_set[k].assigned_flag);
    }
}



static void computing(void) {

    prev_node_num_computing();

    core_assign();

    path_computing();


}

static void priority_decision(void){
    
    if(sysfs_streq(pri_str,"path")){path_priority_computing();}
    if(sysfs_streq(pri_str,"exec")){exec_priority_computing();}
    printk(KERN_INFO "priority_computing() finished!");
    sort_priority_in_task();
}


static void scheduling(void){

    high_task_scheduling();
    low_task_scheduling(); 
}

static int subtask_func(int loop) {
    //printk(KERN_INFO "in subtask_function");
    int i = 0;
    for(i = 0; i < loop;i++){
        ktime_get();
    }
    //printk(KERN_INFO "subtask_func() is ending....");
	return 0;
}


enum hrtimer_restart timer_expire_func( struct hrtimer *timer ) {
    int i = 0;
    for(i = 0; i < CORE_NUM;i++){
        if(&(timers_core[i]) == timer){
            printk(KERN_INFO "wake up task on core[%d]",i);
            wake_up_process(kthreads[i]);
        }
    }
	return HRTIMER_NORESTART;
}


static int core_func(void *data) {
    int core_index = *((int *)data);
    int sub_no;
    int sub_parent;

    ktime_t before,after,diff,exec,per,next,s1,s2;
    int i = 0;
    int subtask_index = 0;
    struct sched_param para;

    printk(KERN_INFO "Core[%d] is in calibrate function\n", core_index);
    int sum= core_set[core_index].subtask_sum_in_core;
    //to calculate the best loop number for each task;
    int num = 0;
    while(num < RECUR_TIME){
      printk(KERN_INFO "num[%d]",num);
      set_current_state(TASK_INTERRUPTIBLE);
      schedule();
      for(i = 0; i < sum; i++){     
        subtask_index = core_set[core_index].subtask_arr_in_core[i]->no;    
        para.sched_priority = subtask_set[subtask_index].priority;
        int ret = sched_setscheduler(kthreads[core_index], SCHED_FIFO, &para);
        


        if(i == 0){ 
          before = ktime_get();
        }
        
        s1 = ktime_get();
        subtask_func(core_set[core_index].subtask_arr_in_core[i]->loop_count);
        s2 = ktime_get();
        if(ktime_compare(ktime_sub(s2,s1),ktime_set(0,core_set[core_index].subtask_arr_in_core[i]->loop_count*MILLION_SEC))>0)
           {printk("[%d]:loop number is large",subtask_index);} 

        //printk(KERN_INFO "subtask[%d] finished",core_set[core_index].subtask_arr_in_core[i]->no);

        if(i == sum - 1){
           after = ktime_get();
           diff = ktime_sub(after,before);
           per = ktime_set(0, task_set[subtask_set[subtask_index].parent_task_no].task_period*MILLION_SEC);
           //struct timespec time_interval1 = ktime_to_timespec(diff);
           struct timespec time_interval2 = ktime_to_timespec(ktime_sub(diff,core_set[core_index].last_release*MILLION_SEC));
           //printk("Real subtask_index[%d]: sec:%d, nanosec%d",subtask_index,time_interval1.tv_sec,time_interval1.tv_nsec);
           printk("Defi subtask_index[%d]: sec:%d, nanosec%d",subtask_index,time_interval2.tv_sec,time_interval2.tv_nsec);
           if(ktime_compare(diff,per) < 0){
             printk("Core[%d]: Current round is finished! Starting next round....",core_index);
             next = ktime_sub(per,diff);
             next = ktime_add(next, 100*MILLION_SEC);
           } 

           if(ktime_compare(diff,per) > 0){
             printk("Core[%d]: Deadline Missing! Starting next round now!",core_index);
             next = ktime_set(0,100*MILLION_SEC);
           } 
           if(num < RECUR_TIME){
              hrtimer_start(&timers_core[core_index],next,HRTIMER_MODE_REL);
              num++;
           }
           else{break;}
        }
      }
    }
    return 0;
}



static int simple_init (void) {

    printk(KERN_INFO "Start initialization.\n");
    //decide mode option : CALIBRATE or SCHEDULE or RUN

    //First step: generate task set and subtask dynamically in init();
    int i,j,k = 0;
    i = 0;
    int temp = 0;
    init();

    if (sysfs_streq(mode_str, "schedule")){
       computing();
       priority_decision();
       scheduling();
       for(i = 0; i < SUBTASK_NUM; i++){
          if(subtask_set[i].assigned_flag == 0){
          printk("The task set is unschedulable[i]",i);
          return 0;}
       }
       for(i = 0; i < CORE_NUM; i++){
           for(j = 0; j < core_set[i].subtask_sum_in_core;j++){
               printk("core[%d]: order[%d], subtask[%d]", i,j,core_set[i].subtask_arr_in_core[j]->no);
           }         
       }
       for(i = 0; i < CORE_NUM;i++){
           kthreads[i] = kthread_create(&core_func, (void *)(&core_set[i].core_no),"kthread");
           kthread_bind(kthreads[i], core_set[i].core_no);
            param.sched_priority = LOW_PRIORITY;
            int ret = sched_setscheduler(kthreads[i], SCHED_FIFO, &param);
            if (ret < 0) {
				printk(KERN_INFO "sched_setscheduler failed!");
				return -1;
            }
            hrtimer_init(&timers_core[i], CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            timers_core[i].function = &timer_expire_func;
       }
       for (i = 0; i < CORE_NUM; i++) {
            wake_up_process(kthreads[i]);
            hrtimer_start(&timers_core[i],ktime_set(0, 10*MILLION_SEC),HRTIMER_MODE_REL);

       }
    }

    if (sysfs_streq(mode_str, "run")){

    
    }


return 0;
}

static void simple_exit (void) {

  int i = 0,j = 0;

    printk(KERN_ALERT "In kernel exit function");
    printk(KERN_ALERT "simple module is being unloaded");

  return;
}

module_init (simple_init);
module_exit (simple_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jiayue Tian");
MODULE_DESCRIPTION ("REAL TIME BEHAVIOR");





































































































