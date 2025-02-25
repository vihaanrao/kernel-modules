[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/mn6KbuEb)
# UIUC CS 423 MP1

Your Name: Vihaan Rao

Your NetID: vihaanr2

## Implementation
The implementation is fairly standard and doesn't have anything special. The `make_proc_entry()` function creates our entry in `/proc/mp1`.

- The write operation (`proc_write_callb`) is called when an userapp registers a process by writing its PID to the entry. The PID is stored in a kernel linked list with an inital CPU time of 0.     

- The read operation (`proc_read_callb`) is called when an userapp reads `/proc/mp1/status` to get a list of all currently registered and alive processes and their respective CPU times. 

The kernel module maintains a global linked list `my_proc_list` to store registered PIDs and their CPU usage which is protected using a mutex to ensure concurrent accesses safely. 

As for the updating workflow for registered processes, a kernel timer (`my_timer`) is triggered every 5 seconds which schedules a workqueue task (`work_handler`) to cupdate the CPU times. `work_handler` iterates over the kernel LL, gets each proc's usertime using the given helper function and removes procs that have terminated. 

Upon unloading the module with `rmmod mp1.ko`, all allocated memory is free'd and proc entries are removed. Also, the timer and workqueue are properly stopped via flushing the workqueue before unloading. 

## How to run
- Compile the Makefile (`make all`)
- Run the `./userapp` to register itself and perform CPU-intensive work. In our case, the current `userapp.c` contains code that calculate the factorial upto a large iteration. 
- Once the workload is done (∼10-14 seconds later), the userapp binary will print out a list of all registered processes in the entry and their respective user time in a format according to the specification.
