#ifndef CONSTANTS_H
#define CONSTANTS_H



#define KEY_POS 2594
#define NUM_GOALS 3
#define NUM_OBSTACLES 10

#define MAX_MSG_LEN 300
#define MAX_LOGGER_LEN 100

#define SEM_PATH "/my_sem_path"     // Customize the path
#define SHMOBJ_PATH "/my_shm_path"

#define SHARED_DATA_SIZE sizeof(struct Position)

struct Position {
    int x;
    int y;
    int key;
};

#endif