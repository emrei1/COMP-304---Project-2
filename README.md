## COMP 304 Project 2 Report - Emre Inceoglu, Ertan Can Guner

### Summary
Our solution is divided into three files:
* `project_2.c` contains the main logic of the program.
* `queue.c` contains logic that implements necessary queue data structures for the project.
* `logger.c` contains the logis that is necessary for logging the simulation results.

Our program begins with the main method generating jobs based on established probability values for the possible jobs (landing, launching, and assembly) until the total simulation time ends. The jobs generated are divided into their own threads executing their methods (i.e. LaunchJob) which input jobs into the landing, launching, and assembly queues (also emergency).

Then, the control tower method (which runs within its own thread initialized from main) checks from thejob queues for which job should be assigned to the pads based on the priorities that are specified in the project specification. To implement the internal logic mutual exclusion is performed, as queues are shared between multiple threads for the program (control tower, pads, and main). The control tower checks for jobs until the total simulation time ends. 

Finally, the pads (A and B) have their own threads that check for jobs every t seconds and sleep their threads based on the job retrieved from the padA (or B) queue.

### Data Structures Used and Concurrency Logic
Our program uses four different queues for landing, launching, assembly, and emergency that are used to store the jobs that are waiting to be sent to the pads from the control tower. They all have their corresponding mutex variables and are locked and unlocked when a thread enters the critical section that modifies these queues. PadA and PadB are also queues used to send jobs to the pads and they also have their own mutexes.
