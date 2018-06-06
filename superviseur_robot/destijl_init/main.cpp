/* 
 * File:   main.c
 * Author: pehladik
 *
 * Created on 23 décembre 2017, 19:45
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/mutex.h>
#include <alchemy/sem.h>
#include <alchemy/queue.h>

#include "./header/functions.h"

// Déclaration des taches
RT_TASK th_server;
RT_TASK th_sendToMon;
RT_TASK th_receiveFromMon;
RT_TASK th_openComRobot;
RT_TASK th_startRobot;
RT_TASK th_move;
RT_TASK th_startCamera;
RT_TASK th_stopCamera;
RT_TASK th_camera;
RT_TASK th_stopRobot;
RT_TASK th_batteryLevel;
RT_TASK th_findArena;
RT_TASK th_computePosition;
RT_TASK th_ripNodejs;
// Déclaration des priorités des taches
int PRIORITY_TSERVER = 30;
int PRIORITY_TOPENCOMROBOT = 20;
int PRIORITY_TMOVE = 10;
int PRIORITY_TSENDTOMON = 25;
int PRIORITY_TRECEIVEFROMMON = 22;
int PRIORITY_TSTARTROBOT = 20;
int PRIORITY_TSTARTCAMERA = 20;
int PRIORITY_STOPCAMERA = 15;
int PRIORITY_CAMERA = 12;
int PRIORITY_STOPROBOT = 15;
int PRIORITY_BATTERYLEVEL = 11;
int PRIORITY_FINDARENA = 21;
int PRIORITY_COMPUTEPOSITION = 21;
int PRIORITY_RIPNODEJS = 5 ;

RT_MUTEX mutex_robotStarted;
RT_MUTEX mutex_move;
RT_MUTEX mutex_connexionOK;
RT_MUTEX mutex_findPosition;
RT_MUTEX mutex_findArena;
RT_MUTEX mutex_oldArena;
RT_MUTEX mutex_newArena;
RT_MUTEX mutex_watchdog;
RT_MUTEX mutex_cameraStarted;
RT_MUTEX mutex_camera;
RT_MUTEX mutex_cmpt;

// Déclaration des sémaphores
RT_SEM sem_barrier;
RT_SEM sem_openComRobot;
RT_SEM sem_serverOk;
RT_SEM sem_startRobot;
RT_SEM sem_searchArena;
RT_SEM sem_startCamera;
RT_SEM sem_nodeDead;
RT_SEM sem_stopCamera;
RT_SEM sem_cameraStarted;
RT_SEM sem_stopRobot;

// Déclaration des files de message
RT_QUEUE q_messageToMon;

int MSG_QUEUE_SIZE = 10;

// Déclaration des ressources partagées
bool robotStarted = false;
char move = DMB_STOP_MOVE;
bool connexionOK = false;
bool findPosition = false;
bool findArena = false;
Arene newArena;
Arene oldArena ;
bool watchdog = false;
bool cameraStarted = false;
Camera cam;
int cmpt = 0;

/**
 * \fn void initStruct(void)
 * \brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void initStruct(void);

/**
 * \fn void startTasks(void)
 * \brief Démarrage des tâches
 */
void startTasks(void);

/**
 * \fn void deleteTasks(void)
 * \brief Arrêt des tâches
 */
void deleteTasks(void);

int main(int argc, char **argv) {
    int err;
    //Lock the memory to avoid memory swapping for this program
    mlockall(MCL_CURRENT | MCL_FUTURE);

    printf("#################################\n");
    printf("#      DE STIJL PROJECT         #\n");
    printf("#################################\n");

    initStruct();
    startTasks();
    rt_sem_broadcast(&sem_barrier);
    pause();
    deleteTasks();

    return 0;
}

void initStruct(void) {
    int err;
    /* Creation des mutex */
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_connexionOK, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_findPosition, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_findArena, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_oldArena, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_newArena, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_watchdog, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_cameraStarted, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_camera, NULL)) {
        printf("Error mutex create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    
    
    

    /* Creation du semaphore */
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_searchArena, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startCamera, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_nodeDead, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_stopCamera, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_cameraStarted, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_stopRobot, NULL, 0, S_FIFO)) {
        printf("Error semaphore create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }   
 
    /* Creation des taches */
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startCamera, "th_startCamera", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_stopCamera, "th_stopCamera", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_stopRobot, "th_stopRobot", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_camera, "th_camera", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_findArena, "th_findArena", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_batteryLevel, "th_batteryLevel", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_computePosition, "th_computePosition", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_ripNodejs, "th_ripNodejs", 0, PRIORITY_TMOVE, 0)) {
        printf("Error task create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    /* Creation des files de messages */
    if (err = rt_queue_create(&q_messageToMon, "toto", MSG_QUEUE_SIZE * sizeof (MessageToRobot), MSG_QUEUE_SIZE, Q_FIFO)) {
        printf("Error msg queue create: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
}

void startTasks() {

    int err;
    
    if (err = rt_task_start(&th_startRobot, &f_startRobot, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }

    if (err = rt_task_start(&th_receiveFromMon, &f_receiveFromMon, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, &f_sendToMon, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_stopCamera, &f_stopCamera, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, &f_openComRobot, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_batteryLevel, &f_batteryLevel, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, &f_move, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
   if (err = rt_task_start(&th_startCamera, &f_startCamera, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_stopRobot, &f_stopRobot, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_computePosition, &f_computePosition, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_ripNodejs, &f_ripNodejs, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_findArena, &f_findArena, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_camera, &f_camera, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_server, &f_server, NULL)) {
        printf("Error task start: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    }
}

void deleteTasks() {
    rt_task_delete(&th_server);
    rt_task_delete(&th_openComRobot);
    rt_task_delete(&th_sendToMon);
    rt_task_delete(&th_receiveFromMon);
    rt_task_delete(&th_startRobot);
    rt_task_delete(&th_move);
    rt_task_delete(&th_startCamera);
    rt_task_delete(&th_stopCamera);
    rt_task_delete(&th_camera);
    rt_task_delete(&th_stopRobot);
    rt_task_delete(&th_batteryLevel);
    rt_task_delete(&th_findArena);
    rt_task_delete(&th_computePosition);
    rt_task_delete(&th_ripNodejs);
    
}
