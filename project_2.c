#include "queue.c"
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "logger.c"

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
double p = 0.2;               // probability of a ground job (launch & assembly)
int snapshot = 30;

#define MAX_WAIT 5

time_t start_time;

int t = 2;

int pad_a_available = 1;
int pad_b_available = 1;

int job_id = 1;

struct Queue *assembly_queue;
struct Queue *launching_queue;
struct Queue *landing_queue;
struct Queue *pad_A;
struct Queue *pad_B;
struct Queue *waiting_queue;
struct Queue *emergency_queue;
struct Queue *pad_A_emergency;
struct Queue *pad_B_emergency;

pthread_mutex_t launching_mutex;
pthread_mutex_t assembly_mutex;
pthread_mutex_t landing_mutex;
pthread_mutex_t pad_A_mutex;
pthread_mutex_t pad_B_mutex;
pthread_mutex_t log_mutex;
pthread_mutex_t id_mutex;
pthread_mutex_t emergency_mutex;
pthread_mutex_t pad_a_emergency_mutex;
pthread_mutex_t pad_b_emergency_mutex;
pthread_mutex_t waiting_mutex;

void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(void *arg); 
void* ExecutePadA(void *arg);
void* ExecutePadB(void *arg);
void* Snapshot(void *arg);

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}

int get_seconds_since_start() {
    return time(NULL) % start_time;
}

int main(int argc,char **argv){
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    // -n (int) => change the snapshot time

    // update parameters to input    
    int i = 1;
    for(i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-n")) {snapshot = atoi(argv[++i]);}
    }
    start_time = time(NULL);
    logger_init();

    srand(seed); // feed the seed

    // initialize data structures
    assembly_queue = malloc(sizeof(*assembly_queue));
    launching_queue = malloc(sizeof(*launching_queue));
    landing_queue = malloc(sizeof(*landing_queue));
    pad_A = malloc(sizeof(*pad_A));
    pad_B = malloc(sizeof(*pad_B));
    waiting_queue = malloc(sizeof(*waiting_queue));
    emergency_queue = malloc(sizeof(*emergency_queue));
    pad_A_emergency = malloc(sizeof(*pad_A_emergency));
    pad_B_emergency = malloc(sizeof(*pad_B_emergency));

    assembly_queue = ConstructQueue(1000);
    launching_queue = ConstructQueue(1000);
    landing_queue = ConstructQueue(1000);
    pad_A = ConstructQueue(1);
    pad_B = ConstructQueue(1);
    waiting_queue = ConstructQueue(1000);
    emergency_queue = ConstructQueue(1000);
    pad_A_emergency = ConstructQueue(1000);
    pad_B_emergency = ConstructQueue(1000);

    // initialize mutex
    pthread_mutex_init(&launching_mutex, NULL);
    pthread_mutex_init(&assembly_mutex, NULL);
    pthread_mutex_init(&landing_mutex, NULL);
    pthread_mutex_init(&pad_A_mutex, NULL);
    pthread_mutex_init(&pad_B_mutex, NULL);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&id_mutex, NULL);
    pthread_mutex_init(&emergency_mutex, NULL);
    pthread_mutex_init(&pad_a_emergency_mutex, NULL);
    pthread_mutex_init(&pad_b_emergency_mutex, NULL);
    pthread_mutex_init(&waiting_mutex, NULL);
    
    // create necessary threads
    pthread_t control_tower_id;
    pthread_create(&control_tower_id, NULL, ControlTower, NULL);
    pthread_t pad_A_id;
    pthread_create(&pad_A_id, NULL, ExecutePadA, NULL);
    pthread_t pad_B_id;
    pthread_create(&pad_B_id, NULL, ExecutePadB, NULL);
    pthread_t first_job;
    pthread_create(&first_job, NULL, LaunchJob, NULL);
    pthread_t snapshot_id;
    pthread_create(&snapshot_id, NULL, Snapshot, NULL);

    // generate new jobs until simulation time. jobs are created according to the probability value p. Possible job types are landing, launching, assembly, and emergency landing. A new thread is created for each generated job.
    while (get_seconds_since_start() < simulationTime) {
        if (get_seconds_since_start() % (40 * t) == 0 && get_seconds_since_start() != 0) {
            int i = 0;  
            for (i = 0; i < 2; i++) {
                pthread_t emergency_id;
                pthread_create(&emergency_id, NULL, EmergencyJob, NULL);
            }   
        }

        double random = (rand() / (double) RAND_MAX);
        if (random > p) {
            // landing 
            pthread_t landing_id;
            pthread_create(&landing_id, NULL, LandingJob, NULL);
        } else {
            if (random > (p / 2)) {
                // new launching job
                pthread_t launching_id;
                pthread_create(&launching_id, NULL, LaunchJob, NULL);
            } else {
                // new assembly job
                pthread_t assembly_id;
                pthread_create(&assembly_id, NULL, AssemblyJob, NULL);
            }
        }
        pthread_sleep(t);
    }
    

    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Job j;
        j.ID = myID;
        j.type = 2;
        Enqueue(myQ, j);
        Job ret = Dequeue(myQ);
        DestructQueue(myQ);
    */
    pthread_join(control_tower_id, NULL);
    pthread_join(pad_A_id, NULL);
    pthread_join(pad_B_id, NULL);
    pthread_join(snapshot_id, NULL);

    DestructQueue(landing_queue);
    DestructQueue(assembly_queue);
    DestructQueue(launching_queue);
    DestructQueue(waiting_queue);
    DestructQueue(pad_A);
    DestructQueue(pad_B);

    return 0;
}

// LandingJob function updates the job queue for landing spacecrafts. it finishes execution when it updates the queue.
void* LandingJob(void *arg){
    Job *job = malloc(sizeof(Job));
    
    pthread_mutex_lock(&landing_mutex);

    pthread_mutex_lock(&id_mutex);
    job->ID = job_id;
    job_id += 1;
    pthread_mutex_unlock(&id_mutex);
    job->type = 1;
    job->created = get_seconds_since_start();
    Enqueue(landing_queue, *job);

    pthread_mutex_unlock(&landing_mutex);
    pthread_exit(0);
}

//Snapshot function prints all the queues every second after the snapshot time is reached 
void* Snapshot(void *arg){
    while(get_seconds_since_start() < simulationTime){
        if(get_seconds_since_start() > snapshot){
            pthread_mutex_lock(&landing_mutex);
            printf("At %d sec landing: ", get_seconds_since_start());
            print_queue(landing_queue);
            printf("\n");
            pthread_mutex_unlock(&landing_mutex);
            
            pthread_mutex_lock(&launching_mutex);
            printf("At %d sec launching: ", get_seconds_since_start());
            print_queue(launching_queue);
            printf("\n");
            pthread_mutex_unlock(&launching_mutex);
            
            pthread_mutex_lock(&assembly_mutex);
            printf("At %d sec assembly: ", get_seconds_since_start());
            print_queue(assembly_queue);
            printf("\n");
            pthread_mutex_unlock(&assembly_mutex);
            
            pthread_mutex_lock(&waiting_mutex);
            printf("At %d sec waiting: ", get_seconds_since_start());
            print_queue(waiting_queue);
            printf("\n");
            pthread_mutex_unlock(&waiting_mutex);
        }
        pthread_sleep(1);
    }
}

// LaunchJob function updates the job queue for launching spacecrafts. it finishes execution when it updates the queue.
void* LaunchJob(void *arg){
    Job *job = malloc(sizeof(Job));

    pthread_mutex_lock(&launching_mutex);
    pthread_mutex_lock(&id_mutex);
    job->ID = job_id;
    job_id += 1;
    pthread_mutex_unlock(&id_mutex);
    
    job->type = 2;
    job->created = get_seconds_since_start();
    Enqueue(launching_queue, *job);

    pthread_mutex_unlock(&launching_mutex);

    pthread_exit(0);
}

// EmergencyJob function updates the job queue for emergency spacecrafts. it finishes execution when it updates the queue.
void* EmergencyJob(void *arg){
    Job *job = malloc(sizeof(Job));
    
    pthread_mutex_lock(&emergency_mutex);

    pthread_mutex_lock(&id_mutex);
    job->ID = job_id;
    job_id += 1;
    pthread_mutex_unlock(&id_mutex);
    job->type = 4;
    job->created = get_seconds_since_start();
    Enqueue(emergency_queue, *job);

    pthread_mutex_unlock(&emergency_mutex);

    pthread_exit(0); 
}

// AssemblyJob function updates the job queue for assembly spacecrafts. it finishes execution when it updates the queue.
void* AssemblyJob(void *arg){
    Job *job = malloc(sizeof(Job));

    pthread_mutex_lock(&assembly_mutex);

    pthread_mutex_lock(&id_mutex);
    job->ID = job_id;
    job_id += 1;
    pthread_mutex_unlock(&id_mutex);
    job->type = 3;
    job->created = get_seconds_since_start();
    Enqueue(assembly_queue, *job);

    pthread_mutex_unlock(&assembly_mutex);

    pthread_exit(0);
}

// the function that controls the air traffic. This job updates the jobs that are given to the padA and padB (they have their own threads and queues). The logic is implemented in a way that prioritizes emergency, landing, and then launching and assembly respectively. Mutual exlusion logic is applied to avoid flawed updates for shared variables in between threads.
void* ControlTower(void *arg){
    int state = 0;
    Job job;
    
    //This is to check if a job requests and send to either one of the pads.
    int enqueued_job = 0;

    // control tower runs until simulation time
    while (get_seconds_since_start() < simulationTime) {
        if(pad_A->size > pad_B->size){
            state = 1;
        }else{
            state = 0;
        }
        // first check if there are any emergency jobs waiting. priority.
        if (emergency_queue->size > 0) {
            pthread_mutex_lock(&emergency_mutex);
            int change = 0;
            //First emergency job is sent to pad B, second one is sent to pad A.
            while(emergency_queue->size != 0) {
                job = Dequeue(emergency_queue);
                if(change){
                    pthread_mutex_lock(&pad_b_emergency_mutex);
                    Enqueue(pad_A_emergency, job);
                    enqueued_job = 1;
                    pthread_mutex_unlock(&pad_b_emergency_mutex);
                }else{
                    pthread_mutex_lock(&pad_b_emergency_mutex);
                    Enqueue(pad_B_emergency, job);
                    pthread_mutex_unlock(&pad_b_emergency_mutex);
                    change = 1;
                }
            }
            enqueued_job = 1; 
            pthread_mutex_unlock(&emergency_mutex);
        } else { 
            // then check other jobs. prioritize landing over assembly and launching.
            // Check if there are any jobs waiting in the waiting queue and check if their wait time is exceeded and check is there is assembly or launching jobs accumulated.
            //If theses conditions are satisfied then the waiting queue is processed.
            while((get_seconds_since_start() - peek(waiting_queue).request) >= MAX_WAIT && (assembly_queue->size <= 3 || launching_queue->size <= 3) && waiting_queue->size > 0){
                pthread_mutex_lock(&waiting_mutex);
                job = Dequeue(waiting_queue);
                pthread_mutex_unlock(&waiting_mutex);
                if(state){ // allocate jobs efficiently in between pads for landing. assign landing job to pad that has fewer jobs 
                    pthread_mutex_lock(&pad_B_mutex);
                    Enqueue(pad_B, job);
                    pthread_mutex_unlock(&pad_B_mutex);
                }else{
                    pthread_mutex_lock(&pad_A_mutex);
                    Enqueue(pad_A, job);
                    pthread_mutex_unlock(&pad_A_mutex);
                }
                //printf("WAIT: land: %d, launch: %d, assemb: %d, wait: %d, a: %d, b: %d\n", landing_queue->size, launching_queue->size, assembly_queue->size,waiting_queue->size, pad_A->size,pad_B->size);
            }

            // For pad B, prioritize landing until conditions are satisfied -> no more jobs are waiting to land or there are 3 or more assembly jobs waiting.
            if(pad_b_available && pad_B->size == 0){ //pad B
                pthread_mutex_lock(&landing_mutex);
                if(landing_queue->size > 0){
                    job = Dequeue(landing_queue);
                    enqueued_job = 1;
                    
                    if(assembly_queue->size >= 3){
                        pthread_mutex_lock(&waiting_mutex);
                        Enqueue(waiting_queue, job);
                        pthread_mutex_unlock(&waiting_mutex);
                    }else{
                        if(state){
                            pthread_mutex_lock(&pad_B_mutex);
                            Enqueue(pad_B, job);
                            pthread_mutex_unlock(&pad_B_mutex);
                        }else{
                            pthread_mutex_lock(&pad_A_mutex);
                            Enqueue(pad_A, job);
                            pthread_mutex_unlock(&pad_A_mutex);
                            
                        }
                    }
                    pthread_mutex_unlock(&landing_mutex);
                }else{
                    pthread_mutex_unlock(&landing_mutex);
                    pthread_mutex_lock(&assembly_mutex);
                    if(assembly_queue->size > 0){
                        job = Dequeue(assembly_queue);
                        enqueued_job = 1;
                        pthread_mutex_lock(&pad_B_mutex);
                        Enqueue(pad_B, job);
                        pthread_mutex_unlock(&pad_B_mutex);
                    }
                    pthread_mutex_unlock(&assembly_mutex);
                }
            }

            // For pad A, prioritize landing until conditions are satisfied -> no more  are waiting to land or there are 3 or more launching jobs waiting.
            if(pad_a_available && pad_A->size == 0){ //pad A
                pthread_mutex_lock(&landing_mutex);
                if(landing_queue->size > 0){
                    job = Dequeue(landing_queue);
                    enqueued_job = 1;
                    if(launching_queue->size >= 3){
                        pthread_mutex_lock(&waiting_mutex);
                        Enqueue(waiting_queue, job);
                        pthread_mutex_unlock(&waiting_mutex);
                    }else{
                        if(state){
                            pthread_mutex_lock(&pad_B_mutex);
                            Enqueue(pad_B, job);
                            pthread_mutex_unlock(&pad_B_mutex);
                        }else{
                            pthread_mutex_lock(&pad_A_mutex);
                            Enqueue(pad_A, job);
                            pthread_mutex_unlock(&pad_A_mutex);
                        }
                    }
                    pthread_mutex_unlock(&landing_mutex);
                }else{
                    pthread_mutex_unlock(&landing_mutex);
                    pthread_mutex_lock(&launching_mutex);
                    if(launching_queue->size > 0){
                        job = Dequeue(launching_queue);
                        enqueued_job = 1;
                        pthread_mutex_lock(&pad_A_mutex);
                        Enqueue(pad_A, job);
                        pthread_mutex_unlock(&pad_A_mutex);
                    }
                    pthread_mutex_unlock(&launching_mutex);
                }
            }
            //printf("land: %d, launch: %d, assemb: %d, wait: %d, a: %d, b: %d\n", landing_queue->size, launching_queue->size, assembly_queue->size,waiting_queue->size, pad_A->size,pad_B->size);
            //If a job is sent to either pads its logged in tower.log
            if (enqueued_job) {
                job.request = get_seconds_since_start();
                log_tower(&job);
                enqueued_job = 0;
            }
        }
        pthread_sleep(t);
    }
}

void* ExecutePadA(void *arg) {
    //Pad a runs until the simulation time is reached.
    //If there a no jobs in its queue then it sleeps 2 seconds.
    while (get_seconds_since_start() < simulationTime){
        if (pad_A->size == 0 && pad_A_emergency->size == 0){ 
            pthread_sleep(t);
        } else{
            //First priority is emergency jobs, then other jobs.
            if (pad_A_emergency->size > 0) {
                pthread_mutex_lock(&pad_a_emergency_mutex);
                pad_a_available = 0;
                Job job = Dequeue(pad_A_emergency);
                job.request = get_seconds_since_start();
                pad_a_available = 1;
                pthread_mutex_unlock(&pad_a_emergency_mutex);
                pthread_sleep(1 * t);
            } else {
                pthread_mutex_lock(&pad_A_mutex); 
                pad_a_available = 0;
                Job job = Dequeue(pad_A);
                job.request = get_seconds_since_start();
                pad_a_available = 1;
                pthread_mutex_unlock(&pad_A_mutex);
                if(job.type == 1){
                    pthread_sleep(1 * t);
                }else{
                    pthread_sleep(2 * t);
                }
                pthread_mutex_lock(&log_mutex);
                job.end = get_seconds_since_start();
                log_event(&job, "A");
                pthread_mutex_unlock(&log_mutex);
             }
        }
    }
    
}


void* ExecutePadB(void *arg) {
    //Pad a runs until the simulation time is reached.
    //If there a no jobs in its queue then it sleeps 2 seconds.
    while (get_seconds_since_start() < simulationTime){
        if(pad_B->size == 0){
            pthread_sleep(t);
        }else{                        
            //First priority is emergency jobs, then other jobs.
            if (pad_B_emergency->size > 0) {
                pthread_mutex_lock(&pad_b_emergency_mutex);
                pad_b_available = 0;
                Job job = Dequeue(pad_B_emergency);
                job.request = get_seconds_since_start();
                pad_b_available = 1;
                pthread_mutex_unlock(&pad_b_emergency_mutex);
                pthread_sleep(1 * t);
            } else {
                pthread_mutex_lock(&pad_B_mutex);
                pad_b_available = 0;
                Job job = Dequeue(pad_B);
                job.request = get_seconds_since_start();
                pad_b_available = 1;
                pthread_mutex_unlock(&pad_B_mutex);
                if(job.type == 1){
                    pthread_sleep(1 * t);
                }else{
                    pthread_sleep(6 * t);
                }
                pthread_mutex_lock(&log_mutex);
                job.end = get_seconds_since_start();
                log_event(&job, "B");
                pthread_mutex_unlock(&log_mutex);
            }
        }
    }
    
}


