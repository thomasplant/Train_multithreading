#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

pthread_mutex_t start_timer;
pthread_mutex_t modify_queue;
pthread_mutex_t crossing;

pthread_cond_t train_ready_to_load;
pthread_cond_t train_ready_to_cross;
pthread_cond_t check_cross;
pthread_cond_t try_to_load;
pthread_cond_t done_crossing;

bool ready_to_load = false;
bool waiting_for_train = true;
bool Crossing_Track = false;
bool ready_to_cross = false;

struct timespec start_time = {0};

// status = 0 unloaded status = 1 loaded, status = 2 crossed.
//priority =1 High, priority = 0; low
//direction =0 Headed East, direction =1 Headed West
struct train {
    int number;
    int load_delay;
    int cross_delay;
    bool direction;
    bool priority;
    int status;
    pthread_cond_t start_crossing;
    bool turn_to_cross;
    struct train *queue;
};

double timespec_to_seconds(struct timespec *ts){
	return ((double) ts->tv_sec) + (((double) ts->tv_nsec) / 1e9);
}

void print_time(struct timespec *ts){
    double seconds = timespec_to_seconds(ts) - timespec_to_seconds(&start_time);
    printf("%02d:%02d:%0d%.1f ", (int)seconds/3600, (int)seconds/60%60, (int)seconds/10%6, seconds -(int)seconds/10 );
}
//This is the function for the trains
void* train_thread(void *t){

    struct train* me =(struct train*) t;

    pthread_mutex_lock(&start_timer);
    while(ready_to_load == 0){
        pthread_cond_wait(&train_ready_to_load, &start_timer);
    }
    pthread_mutex_unlock(&start_timer);

    usleep(me->load_delay*100000);
    
	struct timespec load_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &load_time);
    
    pthread_mutex_lock(&modify_queue);
    while(waiting_for_train == false && Crossing_Track == false){
        pthread_cond_wait(&try_to_load,&modify_queue);
    }
        int i = 0;
        while(me->queue[i].number != -1){
            i++;
        }
        me->queue[i] = *me;
    pthread_cond_broadcast(&train_ready_to_cross);
    print_time(&load_time);
    if(me->direction == 0){
        printf("Train %2d is ready to go East\n", me->number);
    }else{
        printf("Train %2d is ready to go West\n", me->number);
    }
    waiting_for_train = false;
    pthread_mutex_unlock(&modify_queue);


    pthread_mutex_lock(&crossing);
    while(me->queue[i].turn_to_cross == 0){
        pthread_cond_wait(&me->queue[i].start_crossing, &crossing);
    }
    Crossing_Track = true;
	struct timespec crossing_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &crossing_time);
    print_time(&crossing_time);
    if(me->direction == 0){
        printf("Train %2d is ON the main track going East\n", me->number);
    }else{
        printf("Train %2d is ON the main track going West\n", me->number);
    }
    usleep(me->cross_delay*100000);

	struct timespec finish_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &finish_time);
    print_time(&finish_time);
    if(me->direction == 0){
        printf("Train %2d is OFF the main track after going East\n", me->number);
    }else{
        printf("Train %2d is OFF the main track after going West\n", me->number);
    }
    Crossing_Track = false;
    ready_to_cross = false;
    pthread_cond_signal(&done_crossing);
    pthread_mutex_unlock(&crossing);
    
    pthread_exit(NULL);
}

int calc_num_trains(char * filename){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        printf("Invalid File \n");
        exit(1);
    }
    int counter = 0;
    char temp = fgetc(fp);
    while(temp != EOF){
        if(temp == '\n'){
            counter ++;
        }
        temp = fgetc(fp);
    }
    fclose(fp);
    return counter;
}

void trains_start(void){
	pthread_mutex_lock(&start_timer);
	ready_to_load = true;
	pthread_cond_broadcast(&train_ready_to_load);
	pthread_mutex_unlock(&start_timer);
}

void create_trains(char * filename, pthread_t tid[], int num_trains, struct train he[], struct train hw[], struct train le[], struct train lw[]){
   FILE *fp = fopen(filename, "r");
   if(!fp){
    printf("Invalid File\n");
    exit(1);
    }
    char line[10];   
    fgets(line, sizeof(line),fp);

    for(int i=0; i<num_trains; i++){

        char *token =strtok(line, " ");
        struct train* curent= (struct train*)malloc(sizeof(struct train));
        if (curent == NULL){
            printf("Malloc Failed\n");
            exit(1);
        }

        if(token[0] == 'w'){
            curent->direction = 1;
            curent->priority = 0;
            curent->queue = lw;
        } else if (token[0] == 'W'){
            curent->direction = 1;
            curent->priority = 1;
            curent->queue = hw;
        } else if (token[0] == 'e'){
            curent->direction = 0;
            curent->priority = 0;
            curent->queue = le;
        } else if (token[0] == 'E'){
            curent->direction = 0;
            curent->priority = 1;
            curent->queue = he;
        } else{
            printf("invalid train entery\n");
            exit(1);
        }

        token = strtok(NULL, " ");
        curent->load_delay = atoi(token);
        token = strtok(NULL, " ");
        curent->cross_delay = atoi(token);
        curent->number = i;
        curent->status = 0;
        curent->turn_to_cross = 0;

        if(pthread_create(&tid[i], NULL, train_thread, (void *)curent)){
            printf("Failed to create thread\n");
            exit(1);
        }
        fgets(line, sizeof(line),fp);

    }
   
}

int main(int argc, char** argv){
    int num_trains = calc_num_trains(argv[1]);

    if(num_trains == 0){
        printf("Invalid File");
        exit(1);
    }
    struct train he[num_trains];
    for(int i=0; i<num_trains; i++){
        he[i].number = -1;
    }
    struct train hw[num_trains];
    for(int i=0; i<num_trains; i++){
        hw[i].number = -1;
    }
    struct train le[num_trains];
    for(int i=0; i<num_trains; i++){
        le[i].number = -1;
    }
    struct train lw[num_trains];
    for(int i=0; i<num_trains; i++){
        lw[i].number = -1;
    }
    int head_he = 0;
    int head_hw = 0;
    int head_le = 0;
    int head_lw = 0;


    pthread_t tid[num_trains];
    create_trains(argv[1], tid, num_trains, he, hw, le, lw);

	clock_gettime(CLOCK_MONOTONIC, &start_time);
    trains_start();
    int trains_crossed = 0;
    bool first_train = 1;
    bool last_crossed = 0;
    int starvation = 0;

    while(trains_crossed < num_trains){
        pthread_mutex_lock(&crossing);
        while(ready_to_cross == true){
            pthread_cond_wait(&done_crossing, &crossing);
        }
        pthread_mutex_lock(&modify_queue);
        while(lw[head_lw].number == -1 && le[head_le].number == -1 && hw[head_hw].number == -1 && he[head_he].number == -1){
            waiting_for_train = true;
            pthread_cond_wait(&train_ready_to_cross, &modify_queue);
            pthread_cond_broadcast(&try_to_load);
        }

        if(starvation == 3){

            starvation =0;
            if(last_crossed == 0 && lw[head_lw].number != -1){
                lw[head_lw].turn_to_cross = 1;
                pthread_cond_signal(&lw[head_lw].start_crossing);
                head_lw ++;
                last_crossed = 1;
                starvation = 1;
            } else if (le[head_le].number != -1){
                le[head_le].turn_to_cross = 1;
                pthread_cond_signal(&le[head_le].start_crossing);
                head_le ++;
                last_crossed = 1;
                starvation = 1;
            }
        //Only one high prioriy
        } else if(hw[head_hw].number != -1 && he[head_he].number == -1){
            hw[head_hw].turn_to_cross = 1;
            pthread_cond_signal(&hw[head_hw].start_crossing);
            head_hw ++;
            if(last_crossed ==1){
                starvation ++;
            } else{
                last_crossed = 1;
                starvation = 1;
            }
            
        } else if(hw[head_hw].number == -1 && he[head_he].number != -1){
            he[head_he].turn_to_cross = 1;
            pthread_cond_signal(&he[head_he].start_crossing);
            head_he ++;
            if(last_crossed ==0){
                starvation ++;
            } else{
                last_crossed = 0;
                starvation = 1;
            }

        //Two high priority
        }else if(hw[head_hw].number != -1 && he[head_he].number != -1){
            if(last_crossed == 0){
            hw[head_hw].turn_to_cross = 1;
            pthread_cond_signal(&hw[head_hw].start_crossing);
            head_hw ++;
            last_crossed = 1;
            starvation = 1;
            } else {
            he[head_he].turn_to_cross = 1;
            pthread_cond_signal(&he[head_he].start_crossing);
            head_he ++;
            last_crossed = 0;
            starvation = 1;
            }
        
        //low priority only one
        } else if(lw[head_lw].number != -1 && le[head_le].number == -1){
            lw[head_lw].turn_to_cross = 1;
            pthread_cond_signal(&lw[head_lw].start_crossing);
            head_lw ++;
            if(last_crossed ==1){
                starvation ++;
            } else{
                last_crossed = 1;
                starvation = 1;
            }
        } else if(lw[head_lw].number == -1 && le[head_le].number != -1){
            le[head_le].turn_to_cross = 1;
            pthread_cond_signal(&le[head_le].start_crossing);
            head_le ++;
            if(last_crossed ==0){
                starvation ++;
            } else{
                last_crossed = 0;
                starvation = 1;
            }
        
        //Two low priority
        }else if(lw[head_lw].number != -1 && le[head_le].number != -1){
            if(last_crossed == 0){
            lw[head_lw].turn_to_cross = 1;
            pthread_cond_signal(&lw[head_lw].start_crossing);
            head_lw ++;
            last_crossed = 1;
            starvation = 1;
            } else {
            le[head_le].turn_to_cross = 1;
            pthread_cond_signal(&le[head_le].start_crossing);
            head_le ++;
            last_crossed = 0;
            starvation = 1;
            }
        }

        pthread_mutex_unlock(&modify_queue);
        pthread_mutex_unlock(&crossing);

        if(starvation != 0){
        trains_crossed ++;
        ready_to_cross = true;
        }
    }

    for(size_t i = 0; i < num_trains; ++i)
	{
		pthread_join(tid[i], NULL);
	}

    return 0;
}