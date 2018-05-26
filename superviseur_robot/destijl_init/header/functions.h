/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   functions.h
 * Author: pehladik
 *
 * Created on 15 janvier 2018, 12:50
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/mutex.h>
#include <alchemy/sem.h>
#include <alchemy/queue.h>

#include "../../src/monitor.h"    
#include "../../src/robot.h"
#include "../../src/image.h"
#include "../../src/message.h"

extern RT_TASK th_server;
extern RT_TASK th_sendToMon;
extern RT_TASK th_receiveFromMon;
extern RT_TASK th_startCamera;
extern RT_TASK th_stopCamera:
extern RT_TASK th_openComRobot;
extern RT_TASK th_startRobot;
extern RT_TASK th_stopRobot;
extern RT_TASK th_move;
extern RT_TASK th_findArena;
extern RT_TASK th_calculPosition;
extern RT_TASK th_rechargementWatchdog;
extern RT_TASK th_camera;
extern RT_TASK th_niveauBatterie;
extern RT_TASK th_sendCommande;
extern RT_TASK th_ripNodejs;

extern RT_MUTEX mutex_robotStarted;
extern RT_MUTEX mutex_move;
extern RT_MUTEX mutex_connectionOK;
extern RT_MUTEX mutex_watchdog;
extern RT_MUTEX mutex_oldArena;
extern RT_MUTEX mutex_newArena;
extern RT_MUTEX mutex_findArena;
extern RT_MUTEX mutex_findPosition;
extern RT_MUTEX mutex_cmpt;
extern RT_MUTEX mutex_cameraStarted;
extern RT_MUTEX mutex_img;
extern RT_MUTEX mutex_cmpt;

extern RT_SEM sem_barrier;
extern RT_SEM sem_openComRobot;
extern RT_SEM sem_serverOk;
extern RT_SEM sem_startRobot;
extern RT_SEM sem_nodeDead;
extern RT_SEM sem_startCamera;
extern RT_SEM sem_stopCamera;
extern RT_SEM sem_searchArena;
extern RT_SEM sem_killRobot;
extern RT_SEM sem_killCamera;
extern RT_SEM sem_stopRobot;
extern RT_SEM sem_moveStart;
extern RT_SEM sem_ordre;

extern RT_QUEUE q_messageToMon;

extern int etatCommMoniteur;
extern int robotStarted;
extern char move;
extern bool connectionOK;
extern bool watchdog;
extern Image* img;
extern Arene* newArena;
extern Arene* oldArena;
extern bool findArena;
extern bool cameraStarted;
extern int cmpt;

extern int MSG_QUEUE_SIZE;

extern int PRIORITY_TSERVER;
extern int PRIORITY_TOPENCOMROBOT;
extern int PRIORITY_TMOVE;
extern int PRIORITY_TSENDTOMON;
extern int PRIORITY_TRECEIVEFROMMON;
extern int PRIORITY_TSTARTROBOT;

void f_server(void *arg);
void f_sendToMon(void *arg);
void f_receiveFromMon(void *arg);
void f_openComRobot(void * arg);
void f_move(void *arg);
void f_startRobot(void *arg);

#endif /* FUNCTIONS_H */

