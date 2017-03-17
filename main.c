//
//  main.c
//  libexamwriters
//
//  Created by Buğra Ekuklu on 05.04.2016.
//
//  Student number: 150120016
//  MISRA-C compliant.
//
//  Compile with ```gcc -Wall main.c wec.c registrar.c interviewer.c dispatch.c question.c sema.c parser.c -o main -lpthread'''

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "wec.h"
#include "registrar.h"
#include "interviewer.h"

int k_gen = 0;

int main(int argc, const char * argv[]) {
    pid_t registrar_subroutine = 0;
    pid_t interviewer_subroutine = 0;
    int registrar_stack_pointer = 0;
    int interviewer_stack_pointer = 0;
    
    if (argc != 5) {
        fprintf(stderr, "libexamwriters v1.0\nUSAGE:\n$ $LIBEXAMWRITERS_PATH input_file number_of_WEC_members written_exam_duration interview_duration\n");
        
        exit(9);
    }
    
    //  Create a key to get semaphore
    k_gen = ftok(argv[0], 1);
    
    //  Create interprocess semaphores
    int ipc_sema = semget(k_gen + 1, 3, 0700|IPC_CREAT);
    for (int i = 0; i < 3; ++i) {
        semctl(ipc_sema, i, SETVAL, 0);
    }

    printf("ftok: %d, Semaval: %d, err: %s", k_gen + 1, semctl(ipc_sema, 0, GETVAL), strerror(errno));
    
    registrar_subroutine = fork();
    
    if (registrar_subroutine > 0) {
        interviewer_subroutine = fork();
        
        if (interviewer_subroutine > 0) {
            //  Dispatch WEC routine to mother process
            wec_f(argc, argv);
            
            waitpid(interviewer_subroutine, &interviewer_stack_pointer, 0);
            waitpid(registrar_subroutine, &registrar_stack_pointer, 0);
            
            //  Release interprocess semaphores
            for (int i = 0; i < 3; ++i) {
                semctl(ipc_sema, i, IPC_RMID);
            }
            
        } else if (interviewer_subroutine == 0) {
            //  Dispatch interviewer routine to second child
            interviewer_f(argc, argv);
        } else {
            fprintf(stderr, "<#PID: %d> could not register INTERVIEWER_SUBROUTINE.\n", getpid());
            
            kill(registrar_subroutine, SIGKILL);
            
            exit(0);
        }
    } else if (registrar_subroutine == 0) {
        //  Dispatch registrar routine to first child
        registrar_f(argc, argv);
    } else {
        fprintf(stderr, "<#PID: %d> could not register REGISTRAR_SUBROUTINE.\n", getpid());
        
        exit(0);
    }
    
    return 0;
}














