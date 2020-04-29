
[-] task generator
Compiling task generator：

 $: gcc -o task_generator task_generator.c 
 
Execute task generator, run following command:

$: ./task_generator [task_num] [period] [util] [mode] [core]

[-] fork-join module
Execute the fork-join module(mode = calibrate, schedule, run):

$ sudo insmod time_behavior.ko mode=[mode]

[-] list scheduling module
Execute the fork-join module(mode = schedule, run):

$ sudo insmod time_behavior.ko mode=[mode]

To check result:

$ sudo dmesg

To remove the module:

$ sudo -f rmmod time_behavior.ko
