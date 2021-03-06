#include <fstream>
#include "../header/functions.h"

char mode_start;
void write_in_queue(RT_QUEUE *, MessageToMon);

void f_server(void *arg) {
    int err;
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init %s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    err = run_nodejs("/usr/local/bin/node", "/home/pi/Interface_Robot/server.js");

    if (err < 0) {
        printf("Failed to start nodejs: %s\n", strerror(-err));
        exit(EXIT_FAILURE);
    } else {
#ifdef _WITH_TRACE_
        printf("%s: nodejs started\n", info.name);
#endif
        open_server();
        rt_sem_broadcast(&sem_serverOk);
        rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
        connexionOK = true ;
        rt_mutex_release(&mutex_connexionOK);

    }
}

void f_camera(void *arg){
    Jpg imJpg ;
    Image img ;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init camera%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /* PERIODIC START */
#ifdef _WITH_TRACE_
    printf("%s: start period\n", info.name);
#endif
    rt_task_set_periodic(NULL, TM_NOW, 100000);
#ifdef _WITH_TRACE_
    printf("%s: waiting for sem CameraStarted\n", info.name);
#endif
    rt_sem_p(&sem_cameraStarted, TM_INFINITE);
    while (1) {
        rt_task_wait_period(NULL);
        
    rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
    if(!findArena){      
        rt_mutex_release(&mutex_findArena);
        rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
        if(cameraStarted){
            rt_mutex_release(&mutex_cameraStarted);      
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            get_image(&cam, &img) ;
            rt_mutex_release(&mutex_camera); 
            rt_mutex_acquire(&mutex_oldArena, TM_INFINITE);
            draw_arena(&img, &img, &oldArena); 
            rt_mutex_release(&mutex_oldArena);
            compress_image(&img,&imJpg);
            send_message_to_monitor(HEADER_STM_IMAGE,&imJpg);
        } else
            rt_mutex_release(&mutex_cameraStarted);
    } else 
        rt_mutex_release(&mutex_findArena);
    }
}

void f_startCamera(void* arg){
    int err;
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init startCamera%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);
        
#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_startCamera\n", info.name);
#endif
    do {
        
        rt_sem_p(&sem_startCamera, TM_INFINITE);
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        err = open_camera(&cam);
        rt_mutex_release(&mutex_camera);
        if (err == 0) {
#ifdef _WITH_TRACE_
            printf("%s : the camera started \n", info.name);
#endif
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            cameraStarted = true;
            rt_mutex_release(&mutex_cameraStarted);
            rt_sem_v(&sem_cameraStarted);
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_ACK);
            write_in_queue(&q_messageToMon, msg);
        } else {
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_NO_ACK);
            write_in_queue(&q_messageToMon, msg);
        }
        
    }while(1);
}

void f_stopCamera(void* arg){
    
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init stopCamera%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);   
    while(1){
#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_stopCamera\n", info.name);
#endif
        rt_sem_p(&sem_stopCamera, TM_INFINITE);
#ifdef _WITH_TRACE_
    printf("%s : waiting for mutex camera\n", info.name);
#endif
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        close_camera(&cam);
        rt_mutex_release(&mutex_camera);
#ifdef _WITH_TRACE_
    printf("%s : waiting for mutex cameraStarted\n", info.name);
#endif
        rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
        cameraStarted = false;
        rt_mutex_release(&mutex_cameraStarted);
        MessageToMon msg;
        set_msgToMon_header(&msg, HEADER_STM_ACK);
        write_in_queue(&q_messageToMon, msg);
        
    }
}

void f_sendToMon(void * arg) {
int err;
    MessageToMon msg;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init sendToMon%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_serverOk\n", info.name);
#endif
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    
    while (1) {
        rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
        if(connexionOK){
            rt_mutex_release(&mutex_connexionOK);

#ifdef _WITH_TRACE_
        printf("%s : waiting for a message in queue\n", info.name);
#endif
            if (rt_queue_read(&q_messageToMon, &msg, sizeof (MessageToRobot), TM_INFINITE) >= 0) {
  #ifdef _WITH_TRACE_
              printf("%s : message {%s,%s} in queue\n", info.name, msg.header, msg.data);
  #endif

              err = send_message_to_monitor(msg.header, msg.data);

              if(err == -1){
                  rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
                  connexionOK = false;
                  rt_mutex_release(&mutex_connexionOK);
              }    
              free_msgToMon_data(&msg);
              rt_queue_free(&q_messageToMon, &msg);
          } else {
              printf("Error msg queue write: %s\n", strerror(-err));
          }
      } else {
        rt_mutex_release(&mutex_connexionOK);
      }
    }
}

void f_receiveFromMon(void *arg) {
    MessageFromMon msg;
    int err;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init receiveFromMon%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_serverOk\n", info.name);
#endif
    
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    //rt_mutex_acquire(&mutex_connectionOK, TM_INFINITE);
    do {
        //rt_mutex_release(&mutex_connectionOK);
#ifdef _WITH_TRACE_
            printf("%s : connectionOK =  %d\n", info.name, connexionOK);
#endif
        rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
#ifdef _WITH_TRACE_
            printf("%s : mutex connectionOK\n", info.name);
#endif        
        if(connexionOK){
            rt_mutex_release(&mutex_connexionOK);
#ifdef _WITH_TRACE_
        printf("%s : waiting for a message from monitor\n", info.name);
#endif
            err = receive_message_from_monitor(msg.header, msg.data);

            if(err == -1){
                rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
                connexionOK = false ;
                rt_mutex_release(&mutex_connexionOK);
                rt_sem_v(&sem_nodeDead);
            }
            else{
    #ifdef _WITH_TRACE_
            printf("%s: msg {header:%s,data=%s} received from UI\n", info.name, msg.header, msg.data);
    #endif
                if (strcmp(msg.header, HEADER_MTS_COM_DMB) == 0) {
                    if (msg.data[0] == OPEN_COM_DMB) { // Open communication supervisor-robot
    #ifdef _WITH_TRACE_
                    printf("%s: message open Xbee communication\n", info.name);
    #endif
                        rt_sem_v(&sem_openComRobot);
                    }
                    else if(msg.data[0] == CLOSE_COM_DMB){
    #ifdef _WITH_TRACE_
                    printf("%s: message close Xbee communication\n", info.name);
    #endif
                        rt_sem_v(&sem_stopRobot);
                    }
                } else if (strcmp(msg.header, HEADER_MTS_DMB_ORDER) == 0) {
                    if (msg.data[0] == DMB_START_WITHOUT_WD) { // Start robot
                        rt_mutex_acquire(&mutex_watchdog, TM_INFINITE);
                        watchdog = false;
                        rt_mutex_release(&mutex_watchdog);
    #ifdef _WITH_TRACE_
                    printf("%s: message start robot\n", info.name);
    #endif
                        rt_sem_v(&sem_startRobot);

                    } else if (msg.data[0] == DMB_START_WITH_WD){
                        rt_mutex_acquire(&mutex_watchdog, TM_INFINITE);
                        watchdog = true;
                        rt_mutex_release(&mutex_watchdog);
    #ifdef _WITH_TRACE_
                    printf("%s: message start robot\n", info.name);
    #endif   
                        rt_sem_v(&sem_startRobot);
                    }else if(msg.data[0] == DMB_IDLE){
    #ifdef _WITH_TRACE_
                    printf("%s: message idle robot\n", info.name);
    #endif   
                        send_command_to_robot(DMB_IDLE);
                        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                        robotStarted = false ;
                        rt_mutex_release(&mutex_robotStarted);
                    }
                    else if ((msg.data[0] == DMB_GO_BACK)
                            || (msg.data[0] == DMB_GO_FORWARD)
                            || (msg.data[0] == DMB_GO_LEFT)
                            || (msg.data[0] == DMB_GO_RIGHT)
                            || (msg.data[0] == DMB_STOP_MOVE)) {

                        rt_mutex_acquire(&mutex_move, TM_INFINITE);
                        move = msg.data[0];
                        rt_mutex_release(&mutex_move);
    #ifdef _WITH_TRACE_
                    printf("%s: message update movement with %c\n", info.name, move);
    #endif

                    }
                } else if (strcmp(msg.header, HEADER_MTS_CAMERA) == 0){
                    if (msg.data[0] == CAM_OPEN) {
    #ifdef _WITH_TRACE_
                    printf("%s: message start camera\n", info.name);
    #endif   
                        rt_sem_v(&sem_startCamera);
                    } else if(msg.data[0] == CAM_CLOSE){
    #ifdef _WITH_TRACE_
                    printf("%s: message stop camera\n", info.name);
    #endif  
                        rt_sem_v(&sem_stopCamera);
                    }else if(msg.data[0] == CAM_ARENA_CONFIRM){
                        rt_mutex_acquire(&mutex_oldArena, TM_INFINITE);
                        rt_mutex_acquire(&mutex_newArena, TM_INFINITE);
                        oldArena = newArena;
                        rt_mutex_release(&mutex_newArena);
                        rt_mutex_release(&mutex_oldArena);
                        rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
                        findArena = false;
                        rt_mutex_release(&mutex_findArena);
                    }else if(msg.data[0] == CAM_ARENA_INFIRM){
                        rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
                        findArena = false;
                        rt_mutex_release(&mutex_findArena);
                    }else if(msg.data[0] == CAM_COMPUTE_POSITION){
                        rt_mutex_acquire(&mutex_findPosition, TM_INFINITE);
                        findPosition = true;
                        rt_mutex_release(&mutex_findPosition);
                    }
                    else if(msg.data[0] == CAM_STOP_COMPUTE_POSITION){
                        rt_mutex_acquire(&mutex_findPosition, TM_INFINITE);
                        findPosition = false;
                        rt_mutex_release(&mutex_findPosition);
                    }
                    else if(msg.data[0] == CAM_ASK_ARENA){
    #ifdef _WITH_TRACE_
                    printf("%s: message findArena\n", info.name);
    #endif              
                        rt_mutex_acquire(&mutex_findArena, TM_INFINITE);
                        findArena = true;
                        rt_mutex_release(&mutex_findArena);
                        rt_sem_v(&sem_searchArena);
                        
                    }
                }
                
            }
        }
        else
            rt_mutex_release(&mutex_connexionOK);
        
    } while (1);
}

void f_openComRobot(void * arg) {
    int err;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init %s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    while (1) {
#ifdef _WITH_TRACE_
        printf("%s : Wait sem_openComRobot\n", info.name);
#endif
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
#ifdef _WITH_TRACE_
        printf("%s : sem_openComRobot arrived => open communication robot\n", info.name);
#endif
        err = open_communication_robot();
        if (err == 0) {
#ifdef _WITH_TRACE_
            printf("%s : the communication is opened\n", info.name);
#endif
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_ACK);
            write_in_queue(&q_messageToMon, msg);
        } else {
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_NO_ACK);
            write_in_queue(&q_messageToMon, msg);
        }
    }
}

void f_startRobot(void * arg) {
    int err;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init %s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    while (1) {
#ifdef _WITH_TRACE_
        printf("%s : Wait sem_startRobot\n", info.name);
#endif
        rt_sem_p(&sem_startRobot, TM_INFINITE);
#ifdef _WITH_TRACE_
        printf("%s : sem_startRobot arrived => Start robot\n", info.name);
#endif
        err = send_command_to_robot(DMB_START_WITHOUT_WD);
        if (err == 0) {
#ifdef _WITH_TRACE_
            printf("%s : the robot is started\n", info.name);
#endif
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = true;
            rt_mutex_release(&mutex_robotStarted);
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_ACK);
            write_in_queue(&q_messageToMon, msg);
        } else {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = false;
            rt_mutex_release(&mutex_robotStarted);
            MessageToMon msg;
            set_msgToMon_header(&msg, HEADER_STM_NO_ACK);
            write_in_queue(&q_messageToMon, msg);
        }
    }
}

void f_stopRobot(void *arg){
     /* INIT */
    int err ;
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init move%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
#ifdef _WITH_TRACE_
        printf("%s : Wait sem_stopRobot\n", info.name);
#endif
        rt_sem_p(&sem_stopRobot, TM_INFINITE);    
        send_command_to_robot(DMB_IDLE);
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        robotStarted = false ;
        rt_mutex_release(&mutex_robotStarted);
#ifdef _WITH_TRACE_
        printf("%s : Wait envois msg\n", info.name);
#endif
            err = close_communication_robot();
            if(err == 0){
                MessageToMon msg;
                set_msgToMon_header(&msg, HEADER_STM_LOST_DMB);
                write_in_queue(&q_messageToMon, msg);
            }
    
}

void f_move(void *arg) {
    int err;
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init move%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /* PERIODIC START */
#ifdef _WITH_TRACE_
    printf("%s: start period\n", info.name);
#endif
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    while (1) {
#ifdef _WITH_TRACE_
        printf("%s: Wait period \n", info.name);
#endif
        rt_task_wait_period(NULL);
#ifdef _WITH_TRACE_
        printf("%s: Periodic activation\n", info.name);
        printf("%s: move equals %c\n", info.name, move);
#endif
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        if (robotStarted) {
            rt_mutex_release(&mutex_robotStarted);
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            err = send_command_to_robot(move);
            rt_mutex_release(&mutex_move);
            
            rt_mutex_acquire(&mutex_cmpt, TM_INFINITE);
            if(err < 0){
                cmpt++;
                if(cmpt == 3){
                    rt_sem_v(&sem_stopRobot);
                }        
            }
            else{
                cmpt = 0;
            }
            rt_mutex_release(&mutex_cmpt);
#ifdef _WITH_TRACE_
            printf("%s: the movement %c was sent\n", info.name, move);
#endif            
        }else
            rt_mutex_release(&mutex_robotStarted);
    }
}
void f_batteryLevel(void *arg){
    int levelBatterie;

    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init niveauBatterie%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /* PERIODIC START */
#ifdef _WITH_TRACE_
    printf("%s: start period\n", info.name);
#endif
    rt_task_set_periodic(NULL, TM_NOW, 300000000);
    
    while (1) {
#ifdef _WITH_TRACE_
        printf("%s: Wait period \n", info.name);
#endif
        rt_task_wait_period(NULL);
#ifdef _WITH_TRACE_
        printf("%s: Periodic activation\n", info.name);
#endif
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        if (robotStarted) {
            rt_mutex_release(&mutex_robotStarted);
            levelBatterie = send_command_to_robot(DMB_GET_VBAT);
#ifdef _WITH_TRACE_
    printf("%s: niveauBatterie : %d\n", info.name,levelBatterie);
#endif
            rt_mutex_acquire(&mutex_cmpt, TM_INFINITE);
            if(levelBatterie < 0){
                cmpt++;
                if(cmpt == 3)
                    rt_sem_v(&sem_stopRobot);
            }
            else{
                cmpt = 0;
#ifdef _WITH_TRACE_
    printf("%s: send batterielevel\n", info.name);
#endif                
                levelBatterie += 48 ;   
                send_message_to_monitor(HEADER_STM_BAT,  &levelBatterie);
            }
            rt_mutex_release(&mutex_cmpt);
        }
        else
            rt_mutex_release(&mutex_robotStarted);
    }
}

void f_findArena(void *arg){
    int err;
    Jpg imJpg ;
    Image img ;
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init sendCommande%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);
    while(1){    
#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_searchArena\n", info.name);
#endif
        rt_sem_p(&sem_searchArena, TM_INFINITE);
#ifdef _WITH_TRACE_
    printf("%s : waiting for mutex_camera\n", info.name);
#endif              
        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        get_image(&cam, &img) ;
        rt_mutex_release(&mutex_camera); 
#ifdef _WITH_TRACE_
    printf("%s : waiting for mutex_newArena\n", info.name);
#endif            
        rt_mutex_acquire(&mutex_newArena, TM_INFINITE),
#ifdef _WITH_TRACE_
    printf("%s : waiting for detectArena\n", info.name);
#endif           
        err = detect_arena(&img,&newArena);
        if( err ==0){
#ifdef _WITH_TRACE_
    printf("%s : draw arena\n", info.name);
#endif
            draw_arena(&img, &img, &newArena);
            rt_mutex_release(&mutex_newArena);
#ifdef _WITH_TRACE_
    printf("%s : compress Image\n", info.name);
#endif
            compress_image(&img, &imJpg);
#ifdef _WITH_TRACE_
    printf("%s : send imj with camera\n", info.name);
#endif
            send_message_to_monitor(HEADER_STM_IMAGE,&imJpg);
        } else 
            rt_mutex_release(&mutex_newArena);
        
    }
}

void f_computePosition(void *arg){
    int err;
    Jpg imJpg ;
    Image img ;
    Position posTriangle ;
    
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init calcul position%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /* PERIODIC START */
#ifdef _WITH_TRACE_
    printf("%s: start period\n", info.name);
#endif
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    
    while (1) {
#ifdef _WITH_TRACE_
        printf("%s: Wait period \n", info.name);
#endif
        rt_task_wait_period(NULL);
#ifdef _WITH_TRACE_
        printf("%s: Periodic activation\n", info.name);
#endif
#ifdef _WITH_TRACE_
        printf("%s: waiting mutex findPosition\n", info.name);
#endif
        rt_mutex_acquire(&mutex_findPosition, TM_INFINITE);
        if(findPosition){
            rt_mutex_release(&mutex_findPosition);
#ifdef _WITH_TRACE_
        printf("%s: waiting mutex cameraStartred\n", info.name);
#endif
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            if(cameraStarted){
                rt_mutex_release(&mutex_cameraStarted);
#ifdef _WITH_TRACE_
        printf("%s: waiting mutex camera\n", info.name);
#endif
                rt_mutex_acquire(&mutex_camera, TM_INFINITE);
                get_image(&cam, &img) ;
                rt_mutex_release(&mutex_camera);
#ifdef _WITH_TRACE_
        printf("%s: waiting mutex oldArena\n", info.name);
#endif
                rt_mutex_acquire(&mutex_oldArena, TM_INFINITE);
#ifdef _WITH_TRACE_
        printf("%s: detect position\n", info.name);
#endif
                err = detect_position(&img,&posTriangle,&oldArena);
                draw_arena(&img, &img, &newArena);
                rt_mutex_release(&mutex_oldArena);
                if(!(err < 0)){
#ifdef _WITH_TRACE_
        printf("%s: draw Position\n", info.name);
#endif
                    draw_position(&img,&img,&posTriangle);
                    compress_image(&img, &imJpg);
                    send_message_to_monitor(HEADER_STM_IMAGE,&imJpg);
                }
                send_message_to_monitor(HEADER_STM_POS,&posTriangle);
            }else
                rt_mutex_release(&mutex_cameraStarted);
        }
        else
            rt_mutex_release(&mutex_findPosition);
    }
}

void f_ripNodejs(void *arg){
    int err;
    /* INIT */
    RT_TASK_INFO info;
    rt_task_inquire(NULL, &info);
    printf("Init ripNodejs%s\n", info.name);
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
#ifdef _WITH_TRACE_
    printf("%s : waiting for sem_nodeDead\n", info.name);
#endif
    rt_sem_p(&sem_nodeDead, TM_INFINITE);
    rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
    if(!connexionOK){
        rt_mutex_release(&mutex_connexionOK);
        rt_sem_v(&sem_stopRobot);
        rt_sem_v(&sem_stopCamera);
        err = kill_nodejs();
        if(err == 0){
            rt_mutex_acquire(&mutex_connexionOK, TM_INFINITE);
            connexionOK = false;
            rt_mutex_release(&mutex_connexionOK);
        }
            
    }
    else
        rt_mutex_release(&mutex_connexionOK);
}

void write_in_queue(RT_QUEUE *queue, MessageToMon msg) {
    void *buff;
    buff = rt_queue_alloc(&q_messageToMon, sizeof (MessageToMon));
    memcpy(buff, &msg, sizeof (MessageToMon));
    rt_queue_send(&q_messageToMon, buff, sizeof (MessageToMon), Q_NORMAL);
}