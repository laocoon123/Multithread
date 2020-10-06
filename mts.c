#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "linkedlist.h"

#define MICROSECOND 100000 // 1 microsecond to  0.1 second
#define True 1
#define False 0

// maximum 99 threads
pthread_t t[99];

// simply data lock
pthread_mutex_t track_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_modify_mutex = PTHREAD_MUTEX_INITIALIZER;

// convar for start timing all threads together
pthread_mutex_t as_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t all_start_cond = PTHREAD_COND_INITIALIZER;

// convar for check ready trains
pthread_mutex_t tr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t train_ready_cond = PTHREAD_COND_INITIALIZER;

// convar to put a designated train on track
// will use pthread_mutex_init()
pthread_mutex_t rtg_mutex[99];
pthread_cond_t ready_to_go_cond[99];

//boolean
int read_all_bol;

//semaphore
int can_go;
int train_ready;

struct timespec start, stop;

Node* eh;
Node* el;
Node* wh;
Node* wl;

typedef struct Argument{
    char str;
    int id;
    int lt;
    int ct;
    pthread_mutex_t m;
    pthread_cond_t c;
}Argument;

void* train(void* ptr){
    Argument* args = (Argument*) ptr;
    char* dest;
    if(args->str=='E' || args->str=='e'){
    	dest = "East";
    }else{
    	dest = "West";
    }

    //wait until all trains have been read
    pthread_mutex_lock(&as_mutex);
    while(!read_all_bol){
    	pthread_cond_wait(&all_start_cond, &as_mutex);
    }
    pthread_mutex_unlock(&as_mutex);

    //loading
    usleep(args->lt * MICROSECOND);

    clock_gettime(CLOCK_REALTIME, &stop);
    double accum = (stop.tv_sec - start.tv_sec)*1000000+(stop.tv_nsec - start.tv_nsec)/1000;
    printf("00:00:%04.1f Train %2d is ready to go %4s\n", accum/1000000, args->id, dest);

    //add this train to queue
    pthread_mutex_lock(&list_modify_mutex);
    if(args->str=='E'){
    	eh = newNode(eh, args->id, args->lt, args->ct, args->m, args->c);
    }
    else if(args->str=='e'){
    	el = newNode(el, args->id, args->lt, args->ct, args->m, args->c);
    }
    else if(args->str=='W'){
    	wh = newNode(wh, args->id, args->lt, args->ct, args->m, args->c);
    }
    else if(args->str=='w'){
    	wl = newNode(wl, args->id, args->lt, args->ct, args->m, args->c);
    }
    pthread_mutex_unlock(&list_modify_mutex);
    
    //send ready signal to dispatcher
    pthread_mutex_lock(&tr_mutex);
    train_ready++;
    pthread_cond_signal(&train_ready_cond);
    pthread_mutex_unlock(&tr_mutex);

    //wait for train-specific signal
    pthread_mutex_lock(&rtg_mutex[args->id]);
    while(!can_go){
    	pthread_cond_wait(&ready_to_go_cond[args->id], &rtg_mutex[args->id]);
    }
    can_go--;
    pthread_mutex_unlock(&rtg_mutex[args->id]);

    //put it on the track
    pthread_mutex_lock(&track_mutex);

    clock_gettime(CLOCK_REALTIME, &stop);
    accum = (stop.tv_sec - start.tv_sec)*1000000+(stop.tv_nsec - start.tv_nsec)/1000;
    printf("00:00:%04.1f Train %2d is ON the main track going %4s\n", accum/1000000, args->id, dest);

    usleep(args->ct * MICROSECOND);

    clock_gettime(CLOCK_REALTIME, &stop);
    accum = (stop.tv_sec - start.tv_sec)*1000000+(stop.tv_nsec - start.tv_nsec)/1000;
    printf("00:00:%04.1f Train %2d is OFF the main track after going %4s\n", accum/1000000, args->id, dest);
    
    pthread_mutex_unlock(&track_mutex);
	
}

char** tokenize_input(char* line){
	char** token_list = malloc(sizeof(char*)*10);
	char* token = strtok(line, " \t\r\n\a");
	int i = 0;
	while(token != NULL){
		token_list[i] = token;
		i++;
		token = strtok(NULL, " \t\r\n\a");
	}
	token_list[i] = NULL;
	return token_list;
}

void main(int argc, char** argv){
    FILE* file = fopen(argv[1], "r");
    char line[10];
    int id = 0;

    //read input file
    pthread_mutex_lock(&as_mutex);
    read_all_bol = False;
    while(fgets(line, sizeof(line), file)!= NULL){
        char** str = tokenize_input(line);

        Argument* aa = (Argument*)malloc(sizeof(Argument));
        char* str_copy = malloc(strlen(str[0])+1);
        strcpy(str_copy, str[0]);
        aa->str = str_copy[0];
        aa->id = id;
        aa->lt = atoi(str[1]);
        aa->ct = atoi(str[2]);
        aa->m = rtg_mutex[id];
        aa->c = ready_to_go_cond[id];
        pthread_mutex_init(&rtg_mutex[id], NULL);
        pthread_cond_init(&ready_to_go_cond[id], NULL);

        pthread_create(&t[id], NULL, train, (void*)aa);
        id++;
    }
    read_all_bol = True;
    clock_gettime(CLOCK_REALTIME, &start);
    pthread_cond_broadcast(&all_start_cond); //broadcast to all trains to start loading
    pthread_mutex_unlock(&as_mutex);
    
    int count = id;
    train_ready = 0;
    can_go = 0;
	int r4c = 1; //for rule 4c
	char rule4c = ' ';
	int first = True;
    while(count > 0){
    	//wait for a ready train
    	pthread_mutex_lock(&tr_mutex);
    	while(!train_ready){
    		pthread_cond_wait(&train_ready_cond, &tr_mutex);
    	}
    	train_ready--;
    	pthread_mutex_unlock(&tr_mutex);

    	//signal a ready train to cross
    	usleep(MICROSECOND/50);	// wait those same priority trains

    	int thread_id;
    	int min;

    	pthread_mutex_lock(&list_modify_mutex);
    	//dispatcher rules apply
		//rule 4(c), if 3 trains passed, switch
		//printf("r4c is %d\n", r4c);
		if(r4c>2 && rule4c=='w'){
			goto E;
		}else if(r4c>2 && rule4c=='e'){
			goto W;
		}
		//rule 4(b), opposite direction to the previous one
    	char last = 'w';
		int rule4b = False;

		if(last=='E' || last=='e'){
			rule4b = True;
			goto W;
		}else if(last=='W'|| last=='w'){
			rule4b = False;
			goto E;
		}

		E:
		if(eh){
			thread_id = eh->id;
			last = 'E';
			eh = pop(&eh);
			if(first){
				rule4c = 'e';
				first = False;
			}
			if(rule4c=='w'){
				r4c = 1;
			}else{
				r4c++;
			}
			rule4c = 'e';
			goto end;
		}
		if(r4c>2 && rule4c=='w'){goto e;}
		if(r4c>2 && rule4c=='e'){goto e;}
		if(rule4b){ goto w;}
		
		W:
		if(wh){
			thread_id = wh->id;
			last = 'W';
			wh = pop(&wh);
			if(first){
				rule4c = 'w';
				first = False;
			}
			if(rule4c=='e'){
				r4c = 1;
			}else{
				r4c++;
			}
			rule4c = 'w';
			goto end;
		}
		if(r4c>2 && rule4c=='w'){ goto w;}
		if(r4c>2 && rule4c=='e'){ goto w;}
		if(rule4b){ goto E;}

		e:
		if(el){
			thread_id = el->id;
			last = 'e';
			el = pop(&el);
			if(first){
				rule4c = 'e';
				first = False;
			}
			if(rule4c=='w'){
				r4c = 1;
			}else{
				r4c++;
			}
			rule4c = 'e';
			goto end;
		}
		if(r4c>2 && rule4c=='w'){ goto W;}
		if(r4c>2 && rule4c=='e'){ goto end;}
		if(rule4b){ goto end;}

		w:
		if(wl){
			thread_id = wl->id;
			last = 'w';
			wl = pop(&wl);
			if(first){
				rule4c = 'w';
				first = False;
			}
			if(rule4c=='e'){
				r4c = 1;
			}else{
				r4c++;
			}
			rule4c = 'w';
			goto end;
		}
		if(r4c>2 && rule4c=='e'){ goto E;}
		if(r4c>2 && rule4c=='w'){ goto end;}
		if(rule4b){ goto e;}
		
		
		end:

    	pthread_mutex_unlock(&list_modify_mutex);


    	pthread_mutex_lock(&rtg_mutex[thread_id]);
    	can_go++;
    	pthread_cond_signal(&ready_to_go_cond[thread_id]);
    	pthread_mutex_unlock(&rtg_mutex[thread_id]);
    	count--;
    }
    
    //printf("%s\n", "Program end");
    for(int i=0;i<id;i++){
    	pthread_join(t[i], NULL);
    	//printf("train %d is %s\n", i,"end");
    }
    exit(0);
}