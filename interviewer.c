//
//  interviewer.c
//  libexamwriters
//
//  Created by Buğra Ekuklu on 01.05.2016.
//  Copyright © 2016 The Digital Warehouse. All rights reserved.
//

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#include "interviewer.h"
#include "application.h"
#include "sema.h"

#ifndef kNUM_OF_APPLICANTS
#define kNUM_OF_APPLICANTS 3
#endif

extern int k_gen;

int interviewer_f(int argc, const char *argv[]) {
    application_t *applications = NULL;
    int shmid = 0;
    int propagator = 0;
    
    //  Get shared semaphores
    int ipc_sema = semget(k_gen + 1, 0, 0);
    
    //  Wait for registrar
    sema_wait(ipc_sema, 1, 1);
    
    //  Get shared memory
    if ((shmid = shmget(k_gen, sizeof(application_t) * kNUM_OF_APPLICANTS, 0644|IPC_CREAT)) == -1) {
        fprintf(stderr, "Unable to get shared memory.\n");
        
        exit(9);
    }
    
    //  Attach the shared memory section to statically allocated pointer
    if ((applications = shmat(shmid, (void *)0, 0)) == (application_t *)-1) {
        fprintf(stderr, "Unable to attach to shared memory segment.\n");
        
        exit(9);
    }
    
    printf("The Interviewer started interviews.\n");
    
    //  Do the interviews
    for (int i = 0; i < kNUM_OF_APPLICANTS; ++i) {
        sleep(atoi(argv[4]));
        applications[i].interview_score = rand() % 51;
        printf("Interview score of applicant %d is %d.\n", i + 1, applications[i].interview_score);
    }
    
    printf("The Interviewer finished interviews.\n");
    
    //  Detach shared memory section
    if ((propagator = shmdt(applications)) == -1) {
        fprintf(stderr, "Unable to detach from shared memory segment.\n");
        
        exit(0);
    }
    
    //  On callback, provoke the event listener in registrar
    sema_signal(ipc_sema, 2, 1);
    
    return 0;
}