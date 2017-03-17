//
//  wec.c
//  libexamwriters
//
//  Created by Buğra Ekuklu on 01.05.2016.
//  Copyright © 2016 The Digital Warehouse. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

#include "wec.h"
#include "sema.h"
#include "parser.h"

extern int k_gen;

extern void *dispatch_invoke_f(dispatch_t *dispatcher);

question_t questions[100];
unsigned int num_workers = 0;
unsigned int num_questions = 0;
unsigned int num_count = 0;
int sema = 0;
pthread_mutex_t mutex_var = PTHREAD_MUTEX_INITIALIZER;

dispatch_t **readf(int num_workers, const char *file_name);

int wec_f(int argc, const char *argv[]) {
    //  Seed random generator used in task creator
    srand((unsigned int)time(NULL));
    pthread_t threads[100];
    pthread_attr_t attr;
    unsigned int i = 0;
    int ipc_sema = 0;
    
    //  Set POSIX thread detached attribute
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    //  Set number of workers
    num_workers = atoi(argv[2]);
    
    //  Read tasks from input file
    dispatch_t **dispatchers = readf(num_workers, argv[1]);
    
    //  Calculate number of questions going to be created
    for (i = 0; i < num_workers; ++i) {
        if (dispatchers[i]) {
            num_questions += dispatchers[i]->end_pos - dispatchers[i]->start_pos;
        }
    }
    
    //  Get semaphores with OS call
    sema = semget(k_gen, num_questions + 1, 0700|IPC_CREAT);
    ipc_sema = semget(k_gen + 1, 0, 0);
    
    for (i = 0; i < num_questions; ++i) {
        semctl(sema, i, SETVAL, 1);
    }
    
    semctl(sema, num_questions, SETVAL, 0);
    
    unsigned int worker = 0;
    
    //  Allocate and initialize threads and run them all
    for (worker = 0; worker < num_workers; ++worker) {
        //  Error propagator
        int propagator = 0;
        
        //  Create threads via POSIX threads API
        if ((propagator = pthread_create(&threads[worker], &attr, (void *)dispatch_invoke_f, dispatchers[worker]))) {
            fprintf(stderr, "Could not create thread: %s\n", strerror(propagator));
            
            unsigned int a = 0;
            
            for (a = 0; a < num_workers; ++a) {
                free(dispatchers[a]);
            }
            
            free(dispatchers);
            
            unsigned int b = 0;
            
            //  Release semaphores
            for (b = 0; b < num_questions; ++b) {
                semctl(sema, b, IPC_RMID);
            }
            
            return 9;
        }
    }
    
    //  Wait for all threads to finish
    for (i = 0; i < num_workers; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    //  On callback, trigger the event listener on registrar
    sema_signal(ipc_sema, 0, 1);
    
    //  Release objects
    for (i = 0; i < num_workers; ++i) {
        free(dispatchers[i]);
    }
    
    free(dispatchers);
    
    //  Release semaphores
    for (i = 0; i < num_questions + 1; ++i) {
        semctl(sema, i, IPC_RMID);
    }
    
    //  Release mutex
    pthread_mutex_destroy(&mutex_var);
    
    return 0;
}