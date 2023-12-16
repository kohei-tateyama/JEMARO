#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/select.h>
#include "../include/utility.c"
#include "../include/constants.h"


// Signal handler for watchdog
void signal_handler(int signo, siginfo_t *siginfo, void *context){
    if(signo == SIGINT){
        exit(1);
    }
    if(signo == SIGUSR1){
        pid_t wd_pid = siginfo->si_pid;
        kill(wd_pid, SIGUSR2);
    }
}

int main(int argc, char *argv[]) 
{
    printf("HELLO\n");
    // SIGNALS FOR THE WATCHDOG
    struct sigaction sig_act;
    sig_act.sa_sigaction = signal_handler;
    sig_act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGUSR1, &sig_act, NULL);

    

    
    // SENDING THE PID TO WATCHDOG
    pid_t server_pid,wd_pid;
    server_pid=getpid();

    int window_server[2];
    int server_window[2];

    int keyboard_server[2];
    int server_keyboard[2];

    int drone_server[2];
    int server_drone[2];

    int obstacle_server[2];
    int server_obstacle[2];

    int target_server[2];
    int server_target[2];

    int wd_server[2];
    int server_wd[2];

    int send_pipes[NUM_PROCESSES-1][2];
    int rec_pipes[NUM_PROCESSES-1][2];

    //PIPES 
    /*
    WINDOW
    KEYBOARD
    DRONE
    OBSTACLE
    TARGET
    WD
    */

    char server_format[100]= "%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d|%d %d";
    
    // sscanf(argv[1],server_format,   &window_server[0],   &window_server[1],   &server_window[0],   &server_window[1],
    //                                 &keyboard_server[0], &keyboard_server[1], &server_keyboard[0], &server_keyboard[1],
    //                                 &drone_server[0],    &drone_server[1],    &server_drone[0],    &server_drone[1],
    //                                 &obstacle_server[0], &obstacle_server[1], &server_obstacle[0], &server_obstacle[1],
    //                                 &target_server[0],   &target_server[1],   &server_target[0],   &server_target[1],
    //                                 &wd_server[0], &wd_server[1], &server_wd[0], &server_wd[1]); // Get the fds of the pipe to watchdog
    
    sscanf(argv[1],server_format,   &rec_pipes[0][0],   &rec_pipes[0][1], &server_window[0],   &server_window[1],
                                    &rec_pipes[1][0],   &rec_pipes[1][1], &server_keyboard[0], &server_keyboard[1],
                                    &rec_pipes[2][0],   &rec_pipes[2][1], &server_drone[0],    &server_drone[1],
                                    &rec_pipes[3][0],   &rec_pipes[3][1], &server_obstacle[0], &server_obstacle[1],
                                    &rec_pipes[4][0],   &rec_pipes[4][1], &server_target[0],   &server_target[1],
                                    &rec_pipes[5][0],   &rec_pipes[5][1], &server_wd[0],       &server_wd[1]); // Get the fds of the pipe to watchdog
    
    // sscanf(argv[1],server_format,   &rec_pipes[0][0],   &rec_pipes[0][1], &send_pipes[0][0],   &send_pipes[0][1],
    //                                 &rec_pipes[1][0],   &rec_pipes[1][1], &send_pipes[1][0],   &send_pipes[1][1],
    //                                 &rec_pipes[2][0],   &rec_pipes[2][1], &send_pipes[2][0],   &send_pipes[2][1],
    //                                 &rec_pipes[3][0],   &rec_pipes[3][1], &send_pipes[3][0],   &send_pipes[3][1],
    //                                 &rec_pipes[4][0],   &rec_pipes[4][1], &send_pipes[4][0],   &send_pipes[4][1],
    //                                 &rec_pipes[5][0],   &rec_pipes[5][1], &send_pipes[5][0],   &send_pipes[5][1]); // Get the fds of the pipe to watchdog
    
    printf("%d %d\n",rec_pipes[4][1],rec_pipes[4][0]);
    

    // close(server_drone[0]);
    // close(server_keyboard[0]);
    // close(server_window[0]);
    // close(server_obstacle[0]);
    // close(server_target[0]);
    // close(server_wd[0]);




    pid_t all_pids[NUM_PROCESSES];
    fd_set reading;
    //PIDS FOR WATCHDOG
    // while(1){
        FD_ZERO(&reading);
        for(int i=0; i<(NUM_PROCESSES-2);i++){
            FD_SET(rec_pipes[i][0], &reading);
        }
        int max_pipe_fd = -1;
        for (int i = 0; i < (NUM_PROCESSES-2); ++i) {
            if (rec_pipes[i][0] > max_pipe_fd) {
                max_pipe_fd = rec_pipes[i][0];
            }
        }
        int ret_val = select(max_pipe_fd, &reading, NULL, NULL, NULL);
        // printf("%d\n",ret_val);

        for(int j=0; j<(NUM_PROCESSES-2); j++){
            if(ret_val>0){
                FD_ISSET(rec_pipes[j][0],&reading);
                my_read(rec_pipes[j][0],&all_pids[j],rec_pipes[j][1], sizeof(int));
                printf("%d\n",all_pids[j]);
            }
        }
    all_pids[NUM_PROCESSES-2] = getpid();
    my_write(server_wd[1],all_pids,sizeof(all_pids),sizeof(all_pids));
    // }

    



    struct shared_data data, updated_data;
    double drone_pos[6];// Array to store the position of drone
    double obst_pos[20];
    double target_pos[20];
    int key=0;
    int command_force[2]={0,0};
    
    data.key=key;
    memcpy(data.command_force, command_force, sizeof(command_force));
    
    // my_read(target_server[0],&data,server_target[1],sizeof(data));
    // memcpy(target_pos, data.target_pos, sizeof(data.target_pos));
    // printf("TARGET %f %f\n", target_pos[0],target_pos[1]);

    while(1){
        fd_set reading;
        FD_ZERO(&reading);

        //CHANGE THIS READING SECTION INTO A FUNCTION
        for(int i=0; i<6;i++){
            FD_SET(rec_pipes[i][0], &reading);
        }
        for (int i = 0; i < 6; i++) {
            if (rec_pipes[i][0] > max_pipe_fd) {
                max_pipe_fd = rec_pipes[i][0];
            }
        }

        int ret_val= 0;
        ret_val = select(max_pipe_fd, &reading, NULL, NULL, NULL);
        for(int j=0; j<(NUM_PROCESSES-2); j++){
            if(ret_val>0){
                if(FD_ISSET(rec_pipes[j][0],&reading)){
                    my_read(rec_pipes[j][0],&updated_data,rec_pipes[j][1], sizeof(data));

                    //Store shared data into local variable
                    
            
                    switch (j){
                    case 0: //window
                        key=updated_data.key;
                        data.key=updated_data.key;
                        my_write(server_keyboard[1],&data,server_keyboard[0],sizeof(data));
                        
                        break;
                    case 1: //keyboard
                        memcpy(command_force, updated_data.command_force, sizeof(updated_data.command_force));
                        memcpy(data.command_force, updated_data.command_force, sizeof(updated_data.command_force));

                        my_write(server_drone[1],&data,server_drone[0],sizeof(data));
                        break;
                    case 2: //drone
                        memcpy(drone_pos, updated_data.drone_pos, sizeof(updated_data.drone_pos));
                        memcpy(data.drone_pos, updated_data.drone_pos, sizeof(updated_data.drone_pos));

                        my_write(server_target[1],&data,server_target[0],sizeof(data));
                        my_write(server_obstacle[1],&data,server_obstacle[0],sizeof(data));
                        my_write(server_window[1],&data,server_window[0],sizeof(data));
                        break;
                    case 3: //obstacle
                    // printf("OBSTCALE %f %f\n", obst_pos[0],obst_pos[1]);
                        memcpy(obst_pos, updated_data.obst_pos, sizeof(updated_data.obst_pos));
                        memcpy(data.obst_pos, updated_data.obst_pos, sizeof(updated_data.obst_pos));
                        printf("OBSTACLE %f %f\n", obst_pos[0],obst_pos[1]);

                        my_write(server_drone[1],&data,server_drone[0],sizeof(data));
                        my_write(server_window[1],&data,server_window[0],sizeof(data));
                        break;
                    case 4: //target
                        printf("TARGET\n");
                        memcpy(target_pos, updated_data.target_pos, sizeof(updated_data.target_pos));
                        memcpy(data.target_pos, updated_data.target_pos, sizeof(updated_data.target_pos));
                        printf("TARGET %f %f\n", target_pos[0],target_pos[1]);

                        my_write(server_obstacle[1],&data,obstacle_server[0],sizeof(data));
                        my_write(server_drone[1],&data,server_drone[0],sizeof(data));
                        my_write(server_window[1],&data,server_window[0],sizeof(data));
                        break;
                    default:
                        break;
                    }

                    // for(int i=0; i<NUM_PROCESSES-2; i++){
                    //     if(i!=j){ //Dont send it to itself
                    //         my_write(send_pipes[i][1],&data,rec_pipes[i][0],sizeof(data));
                    //     }
                        
                    // }
                }
            }
        }
    // ALL PROCESSES WILL READ FIRST, UPDATE IF NECESSARY, AND THEN WRITE
    // SERVER SHOULD READ EVERYTHING AND WRITE 

    }



    // clean up
    close(server_drone[1]);
    close(server_keyboard[1]);
    close(server_window[1]);
    close(server_obstacle[1]);
    close(server_target[1]);
    close(server_wd[1]);

    close(window_server[0]);
    close(drone_server[0]);
    close(keyboard_server[0]);
    close(target_server[0]);
    close(obstacle_server[0]);
    close(wd_server[0]);

    // close(window_server[1]);
    // close(drone_server[1]);
    // close(keyboard_server[1]);
    // close(target_server[1]);
    // close(obstacle_server[1]);
    // close(wd_server[1]);

    return 0; 
} 