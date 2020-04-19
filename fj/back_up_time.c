#include "head.h"
#include "config.h"
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

int finished = 0;

static void init(void){
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
        subtask_set[i].segment_no = subtask_config_tbl[i][Seg_No];
        subtask_set[i].subtask_no = subtask_config_tbl[i][Sub_No];
	subtask_set[i].relative_ddl = subtask_config_tbl[i][Rela_Ddl];
        subtask_set[i].exec_time = subtask_config_tbl[i][Exec_Time];
        subtask_set[i].priority = subtask_config_tbl[i][Prio];
        subtask_set[i].core_no = subtask_config_tbl[i][Core_No];
        subtask_set[i].release_time = subtask_config_tbl[i][Release_Time];
        subtask_set[i].heavy_flag = subtask_config_tbl[i][Heavy_Flag];
	subtask_set[i].loop_count = subtask_set[i].exec_time * LOOP_COUNT;
        sorted_subtask_arr[i] = i;
        printk(KERN_INFO "subtask[%d]:task_no[%d],segment_no[%d],subtask_no[%d],exec_time[%d],priority[%d],core_no[%d]",i,subtask_set[i].parent_task_no,
	subtask_set[i].segment_no,subtask_set[i].subtask_no,subtask_set[i].exec_time,subtask_set[i].priority,subtask_set[i].core_no);
    }

    //initialize core set;
    for(i = 0; i < CORE_NUM; i++){
        core_set[i].core_no = i;
        core_set[i].core_util = 0;
        core_set[i].subtask_sum_in_core = 0;
        core_set[i].last_release = 0;
    }

    //initialize task_set;
    for(i = 0; i < TASK_NUM; i++){
	task_set[i].task_no = i;
        task_set[i].task_period = task_config[i][Task_Period];
        task_set[i].priority = task_config[i][Task_Prior];
        task_set[i].segment_num = task_config[i][Segment_Num];
        task_set[i].worst_exec_time = task_config[i][W_C_Exec_Time];
        task_set[i].critical_path = task_config[i][Cri_Path];
        task_set[i].util = task_config[i][Util];
	task_set[i].offset = task_config[i][Task_Offset];
        task_set[i].subtask_sum_in_task = 0;
        weight = 2500*task_set[i].worst_exec_time / (task_set[i].task_period*10 - 25*task_set[i].critical_path);

	for(j = 0; j < task_set[i].segment_num;j++){
            task_set[i].seg_arr[j].segment_no = j;
            task_set[i].seg_arr[j].segment_period = -1;
	    task_set[i].seg_arr[j].seg_release = 0;
            task_set[i].seg_arr[j].subtask_num = subtask_num_arr[i][j];
            task_set[i].seg_arr[j].seg_critical_path = subtask_exec_time_config[i][j];
            task_set[i].subtask_sum_in_task += task_set[i].seg_arr[j].subtask_num;
            
      	    if (task_set[i].seg_arr[j].subtask_num*100 > weight){task_set[i].seg_arr[j].seg_heavy = 1;}
            else {task_set[i].seg_arr[j].seg_heavy = 0;}
            for(k = 0; k < task_set[i].seg_arr[j].subtask_num;k++){
                task_set[i].seg_arr[j].subtask_arr[k] = &(subtask_set[temp]);
                task_set[i].seg_arr[j].subtask_arr[k]->heavy_flag = task_set[i].seg_arr[j].seg_heavy;
                task_set[i].seg_arr[j].subtask_arr[k]->utilization = (task_set[i].seg_arr[j].subtask_arr[k]->exec_time * 100) / task_set[i].task_period;
                temp++; 
            }

	   // printk(KERN_INFO "task_no[%d],segment_no[%d],segment_period[%d],seg_critical_path[%d],subtask_sum_in_task[%d],heavy[%d]",task_set[i].task_no,task_set[i].seg_arr[j].segment_no,
            //       task_set[i].seg_arr[j].segment_period,task_set[i].seg_arr[j].seg_critical_path,task_set[i].subtask_sum_in_task,task_set[i].seg_arr[j].seg_heavy);
	}


    }
}
static int subtask_func(int loop){
    //printk(KERN_INFO "in subtask_function");
    int i = 0;
    for(i = 0; i < loop;i++){
        ktime_get();
        if(i % LOOP_COUNT == 0){
        //    printk(KERN_INFO "RUNNING in %d loops", i / LOOP_COUNT);
        }
    }
    //printk(KERN_INFO "subtask_func() is ending....");
	return 0;
}

static void assignment(void){
    int max_util = CORE_NUM * UTILIZATION;
    int i,j = 0;
    int temp = 0;
    for(i = 0; i < SUBTASK_NUM; i++){
        for(j = 0; j < CORE_NUM; j++){
            //printk(KERN_INFO "temp:%d",temp);
            if(core_set[temp].core_util + subtask_set[i].utilization < max_util){
                core_set[temp].core_util = core_set[temp].core_util + subtask_set[i].utilization;
                core_set[temp].subtask_arr_in_core[core_set[temp].subtask_sum_in_core] = i;
                core_set[temp].subtask_sum_in_core += 1;
                subtask_set[i].core_no = temp;
		temp = (temp + 1)%CORE_NUM;
                
                break;
            }
        }
        if(subtask_set[i].core_no == -1){
            printk(KERN_INFO "Cannot assign subtask[%d]", i);
        }
    }
    for(i = 0; i < CORE_NUM; i++){
	printk(KERN_INFO "core_no[%d]:",i);
        for(j = 0; j < core_set[i].subtask_sum_in_core; j++){
            printk(KERN_INFO "    subtask[%d]",core_set[i].subtask_arr_in_core[j]);
	}
    }

}


enum hrtimer_restart timer_expire_calibrate( struct hrtimer *timer ){
    int i = 0;
    for(i = 0; i < CORE_NUM;i++){
        if(&(timers_core[i]) == timer){
            printk(KERN_INFO "wake up task on core[%d]",i);
            wake_up_process(kthreads[i]);
        }
    }
	return HRTIMER_NORESTART;
}

static int calibrate_func(void *data){
    int core_index = *((int *)data);
    int sub_no;
    int sub_parent;
    int sub_index_in_parent;
    int sub_index_in_core;
    ktime_t before,after,diff,exec;
    int i = 0;
    int loop = 0;
    int subtask_index = 0;
    struct sched_param para;
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    printk(KERN_INFO "Core[%d] is in calibrate function\n", core_index);
    int sum= core_set[core_index].subtask_sum_in_core;
    //to calculate the best loop number for each task;
    for(i = 0; i < sum; i++){
        para.sched_priority = 20;
        int ret = sched_setscheduler(kthreads[core_index], SCHED_FIFO, &para);
        
        int subtask_index = core_set[core_index].subtask_arr_in_core[i];
//	Subtask subtask = subtask_set[subtask_index];
        printk(KERN_INFO "   subtask_index[%d]", subtask_index);
//        printk(KERN_INFO "   loop[%d]", subtask_set[subtask_index].loop_count);
        while(subtask_set[subtask_index].loop_count > 0){
            loop = subtask_set[subtask_index].loop_count;
//            printk(KERN_INFO "   loop[%d]", loop);
            //record time before and after the subtask function. 
            before = ktime_get();
            subtask_func(loop);
            after  = ktime_get();
 
            diff = ktime_sub(after,before);
            exec = ktime_set(0,MILLION_SEC * subtask_set[subtask_index].exec_time);
            
           
            if(ktime_compare(diff,exec) > 0){
                sub_no = subtask_set[subtask_index].subtask_no;
                printk(KERN_INFO "subtask[%d] in task[%d]: loop number :[%d] TOO LARGE",subtask_set[subtask_index].subtask_no,subtask_set[subtask_index].parent_task_no,loop);
                subtask_set[subtask_index].loop_count-=500;
            }
     
            else{
                sub_no = subtask_set[subtask_index].subtask_no;
                sub_parent = subtask_set[subtask_index].parent_task_no;
                sub_index_in_parent = subtask_set[subtask_index].index_in_parent;
                //sub_index_in_core = subtask_set[subtask_index].index_in_core;
                printk(KERN_INFO "For subtask[%d] in task[%d]: in core[%d]: the best loop number is %d"
                , sub_no,sub_parent,core_index,loop);
                break;
            }
        }

    }
    return 0;
}

static void ddl_computing(void){
    int i,j,k = 0;
    int light_p = 0;
    int light_c = 0;
    int find_h = 0;
    int cumulative_release = 0;
    for(i = 0; i < TASK_NUM; i++){
        light_p = 0;
        for(j = 0; j < task_set[i].segment_num; j++){
            if(task_set[i].seg_arr[j].seg_heavy == 0){
                light_p += task_set[i].seg_arr[j].seg_critical_path;
                light_c += task_set[i].seg_arr[j].seg_critical_path * task_set[i].seg_arr[j].subtask_num;
            }

        }

        for(j = 0; j < task_set[i].segment_num; j++){
            if(task_set[i].seg_arr[j].seg_heavy == 1){
                int slack = (task_set[i].seg_arr[j].subtask_num * (task_set[i].task_period  - 5/2*light_p))/ (task_set[i].worst_exec_time - light_c);
		//printk(KERN_INFO "task_no[%d], seg_no[%d], slack[%d]",task_set[i].task_no,task_set[i].seg_arr[j].segment_no,slack );    
                find_h = 1;
                for( k = 0; k < task_set[i].seg_arr[j].subtask_num;k++ ){
                     task_set[i].seg_arr[j].subtask_arr[k]->relative_ddl = task_set[i].seg_arr[j].subtask_arr[k]->exec_time *slack;
                    }
       
            }
            if(task_set[i].seg_arr[j].seg_heavy == 0) {
                for( k = 0; k < task_set[i].seg_arr[j].subtask_num;k++ ){
                    task_set[i].seg_arr[j].subtask_arr[k]->relative_ddl = task_set[i].seg_arr[j].subtask_arr[k]->exec_time;
                   }                
            }
            //all light segments
            if(find_h == 0){
                for( k = 0; k < task_set[i].seg_arr[j].subtask_num;k++ ){
                    task_set[i].seg_arr[j].subtask_arr[k]->relative_ddl = task_set[i].seg_arr[j].subtask_arr[k]->exec_time * task_set[i].task_period / task_set[i].critical_path ;
                   }  
            }
            task_set[i].seg_arr[j].segment_period = task_set[i].seg_arr[j].subtask_arr[0]->relative_ddl;
            //printk(KERN_INFO "task_no[%d], seg_no[%d], segment_period[%d]",task_set[i].task_no,task_set[i].seg_arr[j].segment_no,task_set[i].seg_arr[j].segment_period );    
        }

        //set real release time after the task starts

        cumulative_release = task_set[i].offset; 
        for(j = 0; j < task_set[i].segment_num; j++){
      	    task_set[i].seg_arr[j].seg_release = cumulative_release;
            for( k = 0; k < task_set[i].seg_arr[j].subtask_num;k++){
                task_set[i].seg_arr[j].subtask_arr[k]->release_time = cumulative_release;
            }
            cumulative_release += task_set[i].seg_arr[j].segment_period;
            //printk(KERN_INFO "task_no[%d], seg_no[%d], seg_release[%d]",task_set[i].task_no,task_set[i].seg_arr[j].segment_no,task_set[i].seg_arr[j].seg_release );  
        }


    }





}

static void priority_computing(void){
    int i, j, k = 0;
    int highest_priority = 30;
    int temp_priority = 20;
    int temp_rddl = 0;
    int temp_index = 0;

    for(i = 0; i < SUBTASK_NUM; i++){
	subtask_set[j].priority = -1;
    }


    for(i = 0; i < SUBTASK_NUM; i++){
	temp_rddl = 0;
        for (j = 0; j < SUBTASK_NUM; j++ ){
            if(subtask_set[j].priority != -1){
                continue;
            }	
	    //printk(KERN_INFO "find candidate: subtask[%d], temp_rddl[%d],priority:[%d]",j,subtask_set[j].relative_ddl,subtask_set[j].priority);
            
            if(subtask_set[j].relative_ddl > temp_rddl){
                    temp_index = j;
                    temp_rddl = subtask_set[j].relative_ddl;
            }
        }
	subtask_set[temp_index].priority = temp_priority;
       // printk(KERN_INFO "find latest ddl: subtask[%d], temp_rddl[%d],priority:[%d]",temp_index,subtask_set[temp_index].relative_ddl,subtask_set[temp_index].priority);
        
        for(j = 0; j < SUBTASK_NUM; j++){
            if(subtask_set[j].relative_ddl == temp_rddl && subtask_set[j].priority == -1){
                subtask_set[j].priority = temp_priority;
            }
        }	
        temp_priority++;
    }
    //for(i = 0; i < SUBTASK_NUM; i++){
	//printk(KERN_INFO "subtask[%d],relative_ddl:[%d],priority:[%d]",i,subtask_set[i].relative_ddl,subtask_set[i].priority);
    //}

}

static void sort_release_time(void){
    int i, j, k = 0;
    int temp = 0;
    int x = 0;
    int y = 0;

    for(i = 0; i < SUBTASK_NUM ;i++){
        //x = sorted_subtask_arr[i];
        //printk(KERN_INFO "No.[%d], subtask[%d], release_time[%d]\n",i,sorted_subtask_arr[i],subtask_set[sorted_subtask_arr[i]].release_time);
        for(j = i; j < SUBTASK_NUM; j++){
            //y = sorted_subtask_arr[j];

            if(subtask_set[sorted_subtask_arr[j]].release_time < subtask_set[sorted_subtask_arr[i]].release_time){    
                temp = sorted_subtask_arr[i];
                sorted_subtask_arr[i] = sorted_subtask_arr[j];
                sorted_subtask_arr[j] = temp;

            }
            else if(subtask_set[sorted_subtask_arr[j]].release_time == subtask_set[sorted_subtask_arr[i]].release_time){
                if(subtask_set[sorted_subtask_arr[j]].priority > subtask_set[sorted_subtask_arr[i]].priority){
                    temp = sorted_subtask_arr[i];
                    sorted_subtask_arr[i] = sorted_subtask_arr[j];
                    sorted_subtask_arr[j] = temp;
                }

            }
        }

    }

    //for( i=0; i < SUBTASK_NUM; i++){
     // printk(KERN_INFO "No.[%d], subtask[%d], release_time[%d],priority[%d]",i,sorted_subtask_arr[i],subtask_set[sorted_subtask_arr[i]].release_time,subtask_set[sorted_subtask_arr[i]].priority);
    //}
}


static void scheduling(void){
    int i, j, k = 0;
    int temp = 0;
    int x = 0;
    for( i=0; i<SUBTASK_NUM; i++){
        x =  sorted_subtask_arr[i];
        for ( j=0; j<CORE_NUM;j++){
            if(subtask_set[x].release_time <= core_set[temp].last_release){
                
                if((core_set[temp].last_release + subtask_set[x].exec_time) < (subtask_set[x].release_time + subtask_set[x].relative_ddl)){
		    printk(KERN_INFO "core_set[%d].last_release[%d]    subtask_set[%d]:exec_time[%d] release_time[%d] relative_ddl[%d] ",
                                      temp,core_set[temp].last_release,x,subtask_set[x].exec_time,subtask_set[x].release_time,subtask_set[x].relative_ddl);
                    core_set[temp].subtask_arr_in_core[core_set[temp].subtask_sum_in_core] = x;
                    core_set[temp].subtask_sum_in_core +=1;
                    core_set[temp].last_release = core_set[temp].last_release + subtask_set[x].exec_time;
                    subtask_set[x].core_no = temp;
                    //printk(KERN_INFO "subtask[%d] is assigned to core[%d]",x,temp);
		    temp = (temp + 1)%CORE_NUM;
                    break;
                }
            }
            else{
		    //printk(KERN_INFO "core_set[%d].last_release[%d]    subtask_set[%d]:exec_time[%d] release_time[%d] relative_ddl[%d] ",
                    //                  temp,core_set[temp].last_release,x,subtask_set[x].exec_time,subtask_set[x].release_time,subtask_set[x].relative_ddl);
                core_set[temp].subtask_arr_in_core[core_set[temp].subtask_sum_in_core] = x;
                core_set[temp].subtask_sum_in_core +=1;
                core_set[temp].last_release = subtask_set[x].release_time + subtask_set[x].exec_time;
                subtask_set[x].core_no = temp;
                //printk(KERN_INFO "subtask[%d] is assigned to core[%d]",x,temp);
		temp = (temp + 1)%CORE_NUM;
                break;
            }
        }
    }

    for( i=0; i < SUBTASK_NUM; i++){
      printk(KERN_INFO "No.[%d], subtask[%d], release_time[%d], core[%d]",i,sorted_subtask_arr[i],subtask_set[sorted_subtask_arr[i]].release_time,subtask_set[sorted_subtask_arr[i]].core_no);
    }
}




static int lookup_func(struct hrtimer *timer ){
    int i = 0;
    for (i=0; i<TASK_NUM; i++) {
		if (&(task_set[i].task_timer)==timer) {
            printk(KERN_INFO "find subtask with timer :%d",i);
            return i;
        }
    }
        return -1;
}



enum hrtimer_restart timer_expire_run( struct hrtimer *timer ) {

    int index = lookup_func(timer);
    if(index == -1){
        printk(KERN_INFO "Cannot find which subtask has this timer");
        return -1;}

    else{

        //printk(KERN_INFO "TRY TO WAKEUP threads in TASK[%d]",index);
        int i = 0;
        for(i = 0; i < CORE_NUM; i++){
            wake_up_process(task_set[index].thread_arr[i]);
        }
        //ktime_t now = ktime_get();
        //ktime_t interval = ktime_set(0, period*MILLION_SEC);
        //hrtimer_init(&task_set[index].task_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        //task_set[index].task_timer.function = &timer_expire_run;
        //hrtimer_start(&task_set[index].task_timer,interval,HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
    }
}




static int task_func(void *data){
    int i,j,k=0;
    int current_seg = 0;
    int temp = *((int *)data);
    
    int task = temp/CORE_NUM;
    int core = temp%CORE_NUM;
    printk(KERN_INFO "temp[%d] task[%d] in core[%d]",temp,task,core);
    int count = 0;
    //printk(KERN_INFO "task thread beginning: task[%d] in core[%d]",task,core);
    //struct subtask *sub_task = &subtask_set[task];
    struct sched_param para;
    ktime_t before,curr,next,base,interval;


    while(task_set[task].curr_seg[core] < task_set[task].segment_num){

        para.sched_priority = HIGH_PRIORITY;
        sched_setscheduler(task_set[task].thread_arr[core], SCHED_FIFO, &para);
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        current_seg = task_set[task].curr_seg[core];
        //printk(KERN_INFO "task thread waken: task[%d] seg[%d] in core[%d],",task,current_seg,core);
        if(kthread_should_stop()) {
            printk(KERN_INFO "exiting the loop and return.");
            break;
        }
     
        else{
           	       
               if(current_seg == 0){
                  base = ktime_get();
                  next = ktime_add(base,ktime_set(0, task_set[task].seg_arr[current_seg].segment_period*MILLION_SEC));
               }
               if(current_seg > 0){
                  next = ktime_add(next,ktime_set(0, task_set[task].seg_arr[current_seg].segment_period*MILLION_SEC));
               }
               for( i=0; i < task_set[task].seg_arr[current_seg].subtask_num; i++ ){
                    para.sched_priority = task_set[task].seg_arr[current_seg].subtask_arr[i]->priority;
        	    sched_setscheduler(task_set[task].thread_arr[core], SCHED_FIFO, &para);

                    if(task_set[task].seg_arr[current_seg].subtask_arr[i]->core_no == core){
                 
                    subtask_func(task_set[task].seg_arr[current_seg].subtask_arr[i]->loop_count);
                    printk(KERN_INFO "subtask_func finished: task[%d] seg [%d] in core[%d] running subtask[%d]",task,current_seg, core,task_set[task].seg_arr[current_seg].subtask_arr[i]->subtask_no); 
                    count++;
                   }
               }

               if(count == 0){ 
                  if(task == 2 && current_seg == 0 && core==0)
                     printk(KERN_INFO  "task[%d] seg [%d] core[%d] subtask[%d]",task,current_seg,core,task_set[task].seg_arr[current_seg].subtask_arr[0]->core_no);
                  printk(KERN_INFO "No subtask in: task[%d] seg [%d] core[%d] ",task,current_seg, core);
               }

               task_set[task].curr_seg[core]++; 
             
               curr = ktime_get();
               if(current_seg == task_set[task].segment_num){break;}
               if(ktime_compare(curr, next) < 0){
                  //printk(KERN_INFO "Waiting next segment: task[%d] seg [%d] in core[%d]",task,current_seg, core);
                  interval = ktime_sub(next,curr);
                  hrtimer_start(&task_set[task].task_timer, interval,HRTIMER_MODE_REL);
               }
               else{
                  printk("DEADLINE MISS");
                  interval = ktime_set(0,0);
                  hrtimer_start(&task_set[task].task_timer,interval,HRTIMER_MODE_REL);
               } 
        }

    }
    finished++;
    return 0;
}


static int simple_init (void){

    printk(KERN_INFO "Start initialization.\n");
    //decide mode option : CALIBRATE or SCHEDULE or RUN

    //First step: generate task set and subtask dynamically in init();
    int i,j,k = 0;
    i = 0;
    int temp = 0;
    init();
    printk(KERN_ALERT "simple module initialized\n");
    
    //Scheduler Mode:
    if (sysfs_streq(mode_str, "schedule")){
        //calculate the relative deadline;
        ddl_computing();
        //set priority;
        priority_computing();
        //sort release time to decide the order of assignment;
        sort_release_time();
        //assign subtasks to core;
        scheduling();
        for(i = 0; i < SUBTASK_NUM;i++){
            printk(KERN_INFO "subtask[%d]:task_no[%d],segment_no[%d],subtask_no[%d],exec_time[%d],relative_ddl[%d],priority[%d],core_no[%d],release_time[%d],heavy_flag[%d]",i,subtask_set[i].parent_task_no,
	    subtask_set[i].segment_no,subtask_set[i].subtask_no,subtask_set[i].exec_time,subtask_set[i].relative_ddl,subtask_set[i].priority,subtask_set[i].core_no,subtask_set[i].release_time,subtask_set[i].heavy_flag);
       }
    }

    //Calibrate mode:
    if (sysfs_streq(mode_str, "calibrate")){
        assignment();
        printk(KERN_INFO "In CALIBRATE Mode.");
        for(i = 0; i < CORE_NUM;i++){
            kthreads[i] = kthread_create(&calibrate_func, (void *)(&core_set[i].core_no),"kthread");
            kthread_bind(kthreads[i], core_set[i].core_no);
            param.sched_priority = LOW_PRIORITY;
            int ret = sched_setscheduler(kthreads[i], SCHED_FIFO, &param);
            if (ret < 0) {
				printk(KERN_INFO "sched_setscheduler failed!");
				return -1;
            }
            hrtimer_init(&timers_core[i], CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            timers_core[i].function = &timer_expire_calibrate;
        }
        for (i = 0; i < CORE_NUM; i++) {
             wake_up_process(kthreads[i]);
             hrtimer_start(&timers_core[i],ktime_set(0, 10*MILLION_SEC),HRTIMER_MODE_REL);

        }
    
    }

    if (sysfs_streq(mode_str, "run")){
        printk(KERN_INFO "In Run Mode.");
	ddl_computing();
	priority_computing();
	sort_release_time();
	scheduling();
        //for(i = 0; i < SUBTASK_NUM;i++){
        //    printk(KERN_INFO "subtask[%d]:task_no[%d],segment_no[%d],subtask_no[%d],exec_time[%d],relative_ddl[%d],priority[%d],core_no[%d],release_time[%d],heavy_flag[%d]",i,subtask_set[i].parent_task_no,
	//    subtask_set[i].segment_no,subtask_set[i].subtask_no,subtask_set[i].exec_time,subtask_set[i].relative_ddl,subtask_set[i].priority,subtask_set[i].core_no,subtask_set[i].release_time,subtask_set[i].heavy_flag);
        //}
        temp = 0;
        for(i = 0; i < TASK_NUM; i++){
            for(j = 0; j < CORE_NUM;j++){
                task_set[i].curr_seg[j] = 0;
                task_set[i].id[j] = temp;
                //printk("Before thread create:task[%d],core[%d],temp[%d]",temp/CORE_NUM,temp%CORE_NUM,temp);
                task_set[i].thread_arr[j] = kthread_create(&task_func, (void*)(&task_set[i].id[j]),"kthread");
                kthread_bind(task_set[i].thread_arr[j], j);
                param.sched_priority = HIGH_PRIORITY;
                int ret = sched_setscheduler(task_set[i].thread_arr[j], SCHED_FIFO, &param);
                if (ret < 0) {
                    printk(KERN_INFO "sched_setscheduler failed!");
                    return -1;
                }
            temp++;
            }
            hrtimer_init(&task_set[i].task_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            task_set[i].task_timer.function = &timer_expire_run;
            
        }
        for(i = 0; i < TASK_NUM; i++){
            for(j = 0; j < CORE_NUM;j++){
                printk(KERN_INFO "wake up process in task[%d]",i);
                wake_up_process(task_set[i].thread_arr[j]);
            }
            hrtimer_start(&task_set[i].task_timer,ktime_set(0, 10*MILLION_SEC),HRTIMER_MODE_REL);
        }
         
      while(finished < 16);
      printk(KERN_INFO "finished[%d]",finished);

    }


return 0;
}


static void simple_exit (void){

  int i = 0,j = 0;
    printk(KERN_INFO "finished[%d]",finished);
    printk(KERN_ALERT "In kernel exit function");


    printk(KERN_ALERT "simple module is being unloaded");

  return;
}


module_init (simple_init);
module_exit (simple_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jiayue Tian");
MODULE_DESCRIPTION ("REAL TIME BEHAVIOR");






























































































