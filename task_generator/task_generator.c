#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SUBTASK_NUM 60
#define MAX_SEGMENT_NUM 5
#define Priority 20

enum task_feature{Task_Period,Task_Prior, Segment_Num, W_C_Exec_Time, Cri_Path, Util,Task_Offset};
enum task_feature task_fea;



enum subtask_feature{No,Task_No, Seg_No, Sub_No, Exec_Time, Rela_Ddl, Prio,Core_No, Release_Time, Offset, Heavy_Flag, Next_Node, Node_Type,Prev_Num, Criti_Path};



int main(int argc, char* argv[]){

    if(argc!= 6)
      {printf("Wrong input\n"); 
       return -1;
      }
    
    //task number in one task sets: 3,4,5
    int task_num = atoi(argv[1]);
    //task period
    int period = atoi(argv[2]);
    //task utilization
    int util = atoi(argv[3]);
    //mode option: 0->low_utilizaton; 1->high&low mix
    int mode = atoi(argv[4]);
    //core number;
    int core = atoi(argv[5]);



    int i, j, k = 0;   
    int path = 0;
    int exec_time = 0;
    int subtask_num = 0;
    int seg_num = 0;
    time_t seed;
    

    int path_arr[4] = {40,50,70,100};
        
    srand((unsigned) time(&seed));

    int max_util = util * core;  
    int cumulative_util = 0;
    int util_aver = max_util / task_num;

    //record subtask number each task;
    int task_sub_num[task_num];
    int task_sub_in[task_num];

    int task_config[task_num][7];
    int subtask_num_arr[task_num][MAX_SEGMENT_NUM];
    int subtask_exec_time_config[task_num][MAX_SEGMENT_NUM];

    //record segment utilization of each segment
    int seg_util;
    
    int temp = 0;
    int count = 0;    
 
    if(mode == 1 && max_util /100 < 1){
       printf("high&low task can only be generated with utilization >=30");    
       return -1;
    }

    if(mode == 1){
      //high-low utilization mixed tasks
        
       if(max_util < 150){
          srand((unsigned) time(&seed));
          task_config[0][Util] = (int)(rand() % 5) + 1;
       }
       else if(max_util < 200){
          srand((unsigned) time(&seed));
          task_config[0][Util] = (int)(rand() % 15) + 5;
       }
       else if(max_util < 250){
          srand((unsigned) time(&seed));
          task_config[0][Util] = (int)(rand() % 30) + 30;
       }
       else if(max_util < 300){
          srand((unsigned) time(&seed));
          task_config[0][Util] = (int)(rand() % 50) + 40;
       }
       else{
          srand((unsigned) time(&seed));
          task_config[0][Util] = (int)(rand() % 50) + 60;
       }
       task_config[0][Util] = (100 + task_config[i][Util]);
       util_aver = (max_util - task_config[0][Util]) / (task_num-1);
       for(i = 1; i < task_num-1;i++){
          sleep(1);
          srand((unsigned) time(&seed));
          task_config[i][Util] = (int)(rand() % 30) + (-15);
          task_config[i][Util] = (100 + task_config[i][Util]) * util_aver / 100;
          cumulative_util += task_config[i][Util];
      }
      task_config[task_num-1][Util] = max_util - task_config[0][Util] - cumulative_util;
    }



    if(mode == 0) {
    // low-utilizaton tasks only;
      cumulative_util = 0;
    // decide utilization of each task in task set;
      for(i = 0; i < task_num-1;i++){
          sleep(1);
          srand((unsigned) time(&seed));
          task_config[i][Util] = (int)(rand() % 30) + (-15);
          task_config[i][Util] = (100 + task_config[i][Util]) * util_aver / 100;
          cumulative_util += task_config[i][Util];
      }
      task_config[task_num-1][Util] = max_util-cumulative_util;

  }
            
    // decide segment number of each task in task set;
      for(i = 0; i < task_num; i++){
          sleep(1);
          srand((unsigned) time(&seed));
          task_config[i][Segment_Num] = (int)(rand() % 4) + 2;
          task_config[i][Task_Period] = period;
          task_config[i][Task_Prior] = Priority;
          task_config[i][Task_Offset] = 0;
          task_config[i][W_C_Exec_Time] = task_config[i][Util] * task_config[i][Task_Period] / 100;
      }
       
     
     for(i = 0; i < task_num;i++){

        for(j = 0; j < MAX_SEGMENT_NUM;j++){
           subtask_exec_time_config[i][j] = 0;
           subtask_num_arr[i][j] = 0;
         }
     }     

     int num = 0;
     // decide subtask number in each segment each task
       for(i = 0; i < task_num; i++){
           num = 0;
           for(j = 0; j < task_config[i][Segment_Num]-1; j++){
               sleep(1);
               srand((unsigned) time(&seed));
               subtask_num_arr[i][j] = (rand() % 4 ) + 1;
               subtask_num += subtask_num_arr[i][j];
               num += subtask_num_arr[i][j];
           }
           // the last segment only one subtask
           subtask_num_arr[i][j] = 1;
           
           subtask_num += subtask_num_arr[i][j];
           task_sub_num[i] = subtask_num;
            
           num += subtask_num_arr[i][j];
           task_sub_in[i] = num;
       }



     int cum = 0;
     int seg_aver = 0;

     // decide utilization each segment each task ;decide execution time each segment each task
       for(i = 0; i < task_num; i++){
           seg_aver = task_config[i][Util] / task_sub_num[i];
           cum = 0;
           for(j = 0; j < task_config[i][Segment_Num]-1; j++){
               seg_aver = task_config[i][Util] * subtask_num_arr[i][j] / task_sub_in[i];
               printf("seg_aver[%d], subtask_num_arr[%d], task_sub_in[%d]\n",seg_aver,subtask_num_arr[i][j],task_sub_in[i]);
               sleep(1);
               srand((unsigned) time(&seed));
               seg_util = (int)(rand() % 40) +(-15);
               seg_util = (100 + seg_util) * seg_aver / 100;
               subtask_exec_time_config[i][j] = (seg_util * task_config[i][Task_Period] / 100) / subtask_num_arr[i][j];
               if(subtask_exec_time_config[i][j] < 3) subtask_exec_time_config[i][j] = 3;
               cum += seg_util;
           }
           seg_util = task_config[i][Util] - cum;
           subtask_exec_time_config[i][j] = (seg_util * task_config[i][Task_Period] / 100) / subtask_num_arr[i][j] ;
       }
    
     // calculate critical path of each task
        for(i = 0; i < task_num; i++){
           task_config[i][Cri_Path] = 0;
           for(j = 0; j < task_config[i][Segment_Num]; j++){
               task_config[i][Cri_Path] += subtask_exec_time_config[i][j];
           }
        }
    
    int subtask_config_tbl[subtask_num][15];


    // decide next node 
    count = 0;
    for(i = 0; i < task_num;i++){
       for(j = 0; j < task_config[i][Segment_Num];j++){
          for(k = 0; k < subtask_num_arr[i][j];k++){
              srand((unsigned) time(&seed));   
              sleep(1);
              if(j < task_config[i][Segment_Num] -1)
                 subtask_config_tbl[count][Next_Node] = (int)(rand() % (task_sub_num[i]-count-subtask_num_arr[i][j]+k)) + count+subtask_num_arr[i][j]-k;
              else subtask_config_tbl[count][Next_Node] = -1;
              count++;
          }
       }
    }


//print all array message

    printf("int subtask_num_arr[TASK_NUM][MAX_SEGMENT_NUM]={ ");
    for(i = 0; i < task_num;i++){
        printf("{ ");
        for(j = 0; j < MAX_SEGMENT_NUM - 1;j++){
            printf("%d, ",subtask_num_arr[i][j]);
         }
        printf("%d ",subtask_num_arr[i][j]);
        if(i < task_num - 1) printf("},");
        else printf("}");
        
    }
    printf("};\n");



    printf("int subtask_exec_time_config[TASK_NUM][MAX_SEGMENT_NUM]={ ");
    for(i = 0; i < task_num;i++){
        printf("{ ");
        for(j = 0; j < MAX_SEGMENT_NUM - 1;j++){
            printf("%d, ",subtask_exec_time_config[i][j]);
        }
        printf("%d ",subtask_exec_time_config[i][j]);
        if(i < task_num - 1) printf("},");
        else printf("}");
        
    }
    printf("};\n");


    printf("int task_config[TASK_NUM][7]={ \n");
    for(i = 0; i < task_num;i++){
            printf("{ ");
            printf("%d, ",task_config[i][Task_Period]);
            printf("%d, ",task_config[i][Task_Prior]);
            printf("%d, ",task_config[i][Segment_Num]);
            printf("%d, ",task_config[i][W_C_Exec_Time]);
            printf("%d, ",task_config[i][Cri_Path]);
            printf("%d, ",task_config[i][Util]);
            printf("%d }",task_config[i][Task_Offset]);
            if(i < task_num -1) printf(", \n");
            else printf(" ");
            //Task_Period,Task_Prior, Segment_Num, W_C_Exec_Time, Cri_Path, Util,Task_Offset 
    }
    printf("};\n");

    
    count = 0;
    
    printf("int subtask_config_tbl[SUBTASK_NUM][15]={ \n");

    // fill out subtask configuration table
        for(i = 0; i < task_num;i++){
           for(j = 0; j < task_config[i][Segment_Num];j++){
              for(k = 0; k < subtask_num_arr[i][j];k++){
                  printf("{ ");
                  subtask_config_tbl[count][No] = count;
                  subtask_config_tbl[count][Task_No] = i;
                  subtask_config_tbl[count][Seg_No] = j;
                  subtask_config_tbl[count][Sub_No] = k;

                  printf("%d, ",subtask_config_tbl[count][No]);
                  printf("%d, ",subtask_config_tbl[count][Task_No]);
                  printf("%d, ",subtask_config_tbl[count][Seg_No]);
                  printf("%d, ",subtask_config_tbl[count][Sub_No]);
                  printf("%d, ",subtask_exec_time_config[i][j]);
                  printf("%d, ",-1);
                  printf("%d, ",-1);
                  printf("%d, ",-1);
                  printf("%d, ",-1);
                  printf("%d, ",0);
                  printf("%d, ",-1);
                  
                  printf("%d, ",subtask_config_tbl[count][Next_Node]);
                  printf("%d, ",-1);
                  printf("%d, ",0);
                  printf("%d ",-1);
                  if(i == task_num-1 && j == task_config[i][Segment_Num]-1 && k == subtask_num_arr[i][j]-1)
                     printf("} \n");
                  else printf("}, \n");
                  count++;
                  
                  //No,Task_No, Seg_No, Sub_No, Exec_Time, Rela_Ddl, Prio,Core_No, Release_Time, Offset, Heavy_Flag, Next_Node, Node_Type,Prev_Num, Criti_Path
              }
           }
        }
        printf("};\n ");



    printf("int task_sub_num[task_num]={ ");
    for(i = 0; i < task_num -1 ;i++){
            printf("%d, ",task_sub_num[i]);
    }
    printf("%d ",task_sub_num[i]);
    printf("};\n ");


    printf("int task_sub_in[task_num]={ ");
    for(i = 0; i < task_num -1;i++){
            printf("%d, ",task_sub_in[i]);
    }
    printf("%d ",task_sub_in[i]);
    printf("};\n ");




    return 0;


}








