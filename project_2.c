#include "queue.c"
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)

time_t start_time;

int t = 2;

int pad_a_available = 1;
int pad_b_available = 1;

int next_launching_id = 1;
int next_assembly_id = 2;
int next_landing_id = 3;

struct Queue *assembly_queue;
struct Queue *launching_queue;
struct Queue *landing_queue;

pthread_mutex_t launching_mutex;
pthread_mutex_t assembly_mutex;
pthread_mutex_t landing_mutex;

void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(void *arg); 
void* ExecutePadA(void *arg);
void* ExecutePadB(void *arg);

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

    int i = 1;
    for(i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    
    srand(seed); // feed the seed

    pthread_t control_tower_id;
    pthread_create(&control_tower_id, NULL, ControlTower, NULL);


    assembly_queue = malloc(sizeof(*assembly_queue));
    launching_queue = malloc(sizeof(*launching_queue));
    landing_queue = malloc(sizeof(*landing_queue));

    assembly_queue = ConstructQueue(9999999);
    launching_queue = ConstructQueue(9999999);
    landing_queue = ConstructQueue(9999999);

    pthread_mutex_init(&launching_mutex, NULL);
    pthread_mutex_init(&assembly_mutex, NULL);
    pthread_mutex_init(&landing_mutex, NULL);
   
    start_time = time(NULL);

    while (get_seconds_since_start() < simulationTime) {

        double random = (rand() / (double) RAND_MAX);

        if (random <= p) {
            // landing
            pthread_t landing_id;
            pthread_create(&landing_id, NULL, LandingJob, NULL);
        } else {
            if (random > p && random <= (p + ((1 - p) / 2))) {
                // new launching plane
                pthread_t launching_id;
                pthread_create(&launching_id, NULL, LaunchJob, NULL);
            } else {
                // new assembly plane
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

    // your code goes here

    return 0;
}

// the function that creates plane threads for landing
void* LandingJob(void *arg){
    Job job;
    
    pthread_mutex_lock(&landing_mutex);

    job.ID = next_landing_id;
    next_landing_id += 3;
    job.type = 1;
    Enqueue(landing_queue, job);

    pthread_mutex_unlock(&landing_mutex);
    pthread_exit(0);
}

// the function that creates plane threads for departure
void* LaunchJob(void *arg){
    Job job; // = malloc(sizeof(Job));

    pthread_mutex_lock(&launching_mutex);
    
    job.ID = next_launching_id;
    next_launching_id += 3;
    job.type = 2;
    Enqueue(launching_queue, job);

    pthread_mutex_unlock(&launching_mutex);

    pthread_exit(0);
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){

}

// the function that creates plane threads for emergency landing
void* AssemblyJob(void *arg){
    Job job; // = malloc(sizeof(*job));

    pthread_mutex_lock(&assembly_mutex);

    job.ID = next_assembly_id;
    next_assembly_id += 3;
    job.type = 3;
    Enqueue(assembly_queue, job);

    pthread_mutex_unlock(&assembly_mutex);

    pthread_exit(0);
}

// the function that controls the air traffic
void* ControlTower(void *arg){
    while (get_seconds_since_start() < simulationTime) {
        if (pad_b_available == 1) {
            pthread_mutex_lock(&landing_mutex);
            if (landing_queue->size > 0) {
                pad_b_available = 0;
                Job job = Dequeue(landing_queue);
                int id = job.ID;
                printf("PAD B: landing job is currently under execution with job id %d\n", id);
                pthread_t new_landing_thread;
                int landing = 0;
                pthread_create(&new_landing_thread, NULL, ExecutePadB, (void*)&landing);
                pthread_mutex_unlock(&landing_mutex);
            } else {
                pthread_mutex_unlock(&landing_mutex);
                pthread_mutex_lock(&assembly_mutex);
                if (assembly_queue->size > 0) {
                    pad_b_available = 0;
                    Job job = Dequeue(assembly_queue);
                    int id = job.ID;
                    printf("PAD B: assembly job is currently under execution with job id %d\n", id);
                    pthread_t new_assembly_thread;
                    int assembly = 1;
                    pthread_create(&new_assembly_thread, NULL, ExecutePadB, (void*)&assembly);
                }
                pthread_mutex_unlock(&assembly_mutex);

            }
        }

        if (pad_a_available == 1) {
            pthread_mutex_lock(&landing_mutex);
            if (landing_queue->size > 0) {
                pad_a_available = 0;
                Job job = Dequeue(landing_queue);
                int id = job.ID;
                printf("PAD A: landing job is currently under execution with job id %d\n", id);
                pthread_t new_landing_thread;
                int landing = 0;
                pthread_create(&new_landing_thread, NULL, ExecutePadA, (void*)&landing);
                pthread_mutex_unlock(&landing_mutex);
            } else {
                pthread_mutex_unlock(&landing_mutex);
                pthread_mutex_lock(&launching_mutex);
                if (launching_queue->size > 0) {
                    pad_a_available = 0;
                    Job job = Dequeue(launching_queue);
                    int id = job.ID;
                    printf("PAD A: launching job is currently under execution with job id %d\n", id);
                    pthread_t new_launching_thread;
                    int launching = 1;
                    pthread_create(&new_launching_thread, NULL, ExecutePadA, (void*)&launching);
                }
                pthread_mutex_unlock(&launching_mutex);
            }
        }
    }
}

void* ExecutePadA(void *arg) {
    int type = *(int*)arg;
    if (type == 0) { // execute landing
        pthread_sleep(1 * t);
    } else { // execute launching
        pthread_sleep(2 * t);
    }
    pad_a_available = 1;
}


void* ExecutePadB(void *arg) {
    int type = *(int*)arg;
    if (type == 0) { // execute landing
        pthread_sleep(1 * t);
    } else { // execute assembly
        pthread_sleep(6 * t);
    }
    pad_b_available = 1;
}


