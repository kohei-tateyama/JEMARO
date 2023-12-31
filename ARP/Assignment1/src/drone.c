
// window.c
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "../include/constants.h"
#include <signal.h>

void signal_handler(int signo, siginfo_t *siginfo, void *context){
    if(signo == SIGINT){
        exit(1);
    }
    if(signo == SIGUSR1){
        pid_t wd_pid = siginfo->si_pid;
        kill(wd_pid, SIGUSR2);
    }
}

double calc_position(double force,double x1,double x2){
    double x;
    x= (force*T*T-M*x2+2*M*x1+K*T*x1)/(M+K*T); //Eulers method

    // Dont let it go outside of the window
    if(x<0){
        return 0;
    }else if(x>BOARD_SIZE){
        return BOARD_SIZE;
    }

    return x;
}

// Get the new position using calc_function and store the previous positions
double update_pos(double* position, int* xy){
    double new_posx,new_posy;
    new_posx=calc_position(xy[0],position[4],position[2]);
    new_posy=calc_position(xy[1],position[5],position[3]);

    // update position
    for(int i=0; i<4; i++){
        position[i]=position[i+2];}
    position[4]=new_posx;
    position[5]=new_posy;
    return *position;
}

int main(int argc, char *argv[]) {

    // SIGNALS
    struct sigaction signal;
    signal.sa_sigaction = signal_handler;
    signal.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signal, NULL);
    sigaction(SIGUSR1, &signal, NULL);

    // VARIABLES
    int xy[2]; // xy = force direction of x,y as such -> [0,1]
    double position[6];
    int first=0;

    // PIPES
    int keyboard_drone[2],wd_drone[2];
    pid_t drone_pid;
    drone_pid=getpid();
    sscanf(argv[1],"%d %d|%d %d", &keyboard_drone[0], &keyboard_drone[1], &wd_drone[0],&wd_drone[1]);
    close(keyboard_drone[1]);
    close(wd_drone[0]);
    write(wd_drone[1], &drone_pid, sizeof(drone_pid));
    close(wd_drone[1]);

    int flags = fcntl(keyboard_drone[0], F_GETFL); // make the read non blocking so the drone can move without user input 
    fcntl(keyboard_drone[0], F_SETFL, flags | O_NONBLOCK);


    // SHARED MEMORY STUFF
    int shared_seg_size = (1 * sizeof(position));
    sem_t * sem_id = sem_open(SEM_PATH, 0);
    int shmfd  = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    void* shm_ptr = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }


    while (1) {
        // 3 Receive command force from keyboard_manager
        ssize_t bytesRead = read(keyboard_drone[0], xy, sizeof(xy)); 

        // Wait until user's first input
        if (first==0){ 
            sem_wait(sem_id);
            memcpy(position,shm_ptr,shared_seg_size); // 1 Get the initial position of the drone
            sem_post(sem_id);

            if (bytesRead<0){ 
                if (errno != EAGAIN) {
                    perror("reading error");
                    exit(EXIT_FAILURE);
                }
            }else if(bytesRead>0){ // User's first input
                update_pos(position,xy);
                first++;
            }
        }else{ // After first second input
            update_pos(position,xy); 
        }
    
        // 4
        // Send updated drone position to window via shared memory
        sem_wait(sem_id);
        memcpy(shm_ptr, position, shared_seg_size);
        sem_post(sem_id);



        usleep(400000);
        
    }

    // Clean up
    close(keyboard_drone[0]);
    shm_unlink(SHM_PATH);
    sem_close(sem_id);
    sem_unlink(SEM_PATH);

    return 0;
}
