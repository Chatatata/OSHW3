//
//  registrar.c
//  libexamwriters
//
//  Created by Buğra Ekuklu on 01.05.2016.
//  Copyright © 2016 The Digital Warehouse. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>

#include "registrar.h"
#include "application.h"
#include "sema.h"

#ifndef kNUM_OF_APPLICANTS
#define kNUM_OF_APPLICANTS 3
#endif

extern int k_gen;

int registrar_f(int argc, const char *argv[]) {
    application_t *applications = NULL;
    int shmid = 0;
    int propagator = 0;
    
    //  Seed random generator used in task creator
    srand((unsigned int)time(NULL));
    
    //  Get shared semaphores
    int ipc_sema = semget(k_gen + 1, 3, 0);

    printf("k_gen: %d, Semaval: %d, err: %s\n", k_gen + 1, semctl(ipc_sema, 0, GETVAL), strerror(errno));
    
    //  Wait for WEC to finish exporting questions
    sema_wait(ipc_sema, 0, 1);
    
    //  Get shared memory
    if ((shmid = shmget(k_gen, sizeof(application_t) * kNUM_OF_APPLICANTS, 0700|IPC_CREAT)) == -1) {
        fprintf(stderr, "Unable to get shared memory.\n");
        
        exit(9);
    }
    
    //  Attach the shared memory section to statically allocated pointer
    if ((applications = shmat(shmid, (void *)0, 0)) == (application_t *)-1) {
        fprintf(stderr, "Unable to attach to shared memory segment.\n");
        
        exit(9);
    }
    
    //  Initialize shared memory
    memset(applications, 0, sizeof(application_t) * kNUM_OF_APPLICANTS);
    
    printf("The Registrar started the written exam.\n");
    
    //  Sleep for given time (do exam)
    sleep(atoi(argv[3]));
    
    //  Give random exam scores to applicants
    for (int i = 0; i < kNUM_OF_APPLICANTS; ++i) {
        applications[i].exam_score = rand() % 51;
        printf("Written exam score of applicant %d is %d.\n", i + 1, applications[i].exam_score);
    }
    
    printf("The Registrar finished the written exam.\n");
    
    //  Detach shared memory section
    if ((propagator = shmdt(applications)) == -1) {
        fprintf(stderr, "Unable to detach from shared memory segment.\n");
        
        exit(9);
    }
    
    //  Emit signal to interviewer to start its job
    sema_signal(ipc_sema, 1, 1);
    
    //  Wait for interviewer to finish its job
    sema_wait(ipc_sema, 2, 1);
    
    //  Remap pointer to shared memory
    if ((applications = shmat(shmid, (void *)0, 0)) == (application_t *)-1) {
        fprintf(stderr, "Unable to attach to shared memory segment.\n");
        
        exit(9);
    }
    
    printf("The Registrar started calculating total scores.\n");
    
    //  We are assuming that this process will take 3 hours
    sleep(3);
    
    for (int i = 0; i < kNUM_OF_APPLICANTS; ++i) {
        applications[i].total_score = applications[i].exam_score + applications[i].interview_score;
        printf("Total score of applicant %d is %d.\n", i + 1, applications[i].total_score);
    }
    
    printf("The Registrar finished calculating total scores.\n");
    
    if ((propagator = shmdt(applications)) == -1) {
        fprintf(stderr, "Unable to detach from shared memory segment.\n");
        
        exit(9);
    }
    
    return 0;
}
