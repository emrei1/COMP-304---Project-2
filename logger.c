#include <stdio.h>
#include <stdlib.h>

void logger_init() {
  FILE *fp = fopen("events.log", "wb");
  fprintf(fp, "EVENTID STATUS REQUEST END TURNAROUND PAD\n");
  fprintf(fp, "__________________________________________________________\n");
  fclose(fp);

  fp = fopen("tower.log", "wb");
  fprintf(fp, "EVENTID STATUS REQUEST\n");
  fprintf(fp, "__________________________________________________________\n");
  fclose(fp);
}

//Logs the jobs thats been completed by the pads.
void log_event(Job *job, char* pad) {
  char* type;
  if(job->type == 1){
    type = "L";
  }else if(job->type == 2){
    type = "D";
  }else if(job->type == 3){
    type = "A";
  }else{
    type = "E";
  }
  FILE *fp = fopen("events.log", "a");
  fprintf(fp, "%d\t%s\t%d\t%d\t%d\t%s\n", job->ID, type, job->request, job->end, job->end - job->request, pad);
  fclose(fp);
}

//Logs the jobs that came through the control tower
void log_tower(Job *job) {
  char* type;
  if(job->type == 1){
    type = "D";
  }else if(job->type == 2){
    type = "L";
  }else if(job->type == 3){
    type = "A";
  }else{
    type = "E";
  }
  FILE *fp = fopen("tower.log", "a");
  fprintf(fp, "%d\t%s\t%d\n", job->ID, type, job->request);
  fclose(fp);
}
