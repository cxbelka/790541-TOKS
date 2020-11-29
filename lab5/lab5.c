#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

#define PC_NUM 3 //количество компов

pthread_t threads[PC_NUM+1];//массив живых тредов

int chan [PC_NUM*2+1];
//int bloc = 0;

typedef struct mess {//структура первого блока заголовка сообщения (без разделителей)
    int AC;
    pthread_t DA;
    pthread_t SA;
} struct_mes;

const char M_SD = '\xFF';
const char M_ED = '\xFE';



void master_sender(void* pipe_par) {//функция мастера генерации маркеров
    int *pipe = (int*)pipe_par;
    int tx = *pipe;
    int rx = *(pipe+1);
    //printf ("send marker \n");
    sleep(1);

    char head[512];
    sprintf(head, "SENDER %lu : ", pthread_self());

    printf("%s TX = %u, RX = %u \n", head, tx, rx);

    struct_mes marker;
    marker.AC = 0;
    marker.DA = 0;
    marker.SA = 0;

    while (true)
    {
        printf("%s SEND MARKER\n", head);
        write(tx, &M_SD, 1);
        write(tx, &marker, sizeof(marker));
        write(tx, &M_ED, 1);
        sleep(PC_NUM*2+1);
        //break;
    }
    
}

void pc (void* pipe_param){
    sleep(1);

    int *pipe = (int*) pipe_param;
    int tx = pipe[0];
    int rx = pipe[1];

    pthread_t self_id = pthread_self();
    int len_chan;
    int number_thread = 0;

    char* buf;
    struct_mes mess_field;

    buf = (char*)malloc(1024);
    memset(buf,0,1024);

    char head[512];
    sprintf(head, "thread %lu : ", self_id);

    if (self_id == threads[0]){
        //printf("%s MASTER \n", head);
        pthread_create(&threads[PC_NUM], NULL, master_sender, pipe_param);
    }

    printf("%s TX = %u, RX = %u %s \n", head, tx, rx, (self_id == threads[0] ? "| MASTER" : ""));

    int flag;// флаг отбрасывания сообщения
    
    while(true){

        //if (self_id == threads[0]){printf("%s This is master-thread \n", head);}

        memset(buf, 0, 1024);
        mess_field.AC = 0;
        mess_field.DA = 0;
        mess_field.SA = 0;
        flag = 0;
        //printf("tread id %u \n", self_id );
        
        len_chan = read(rx, &buf[0],1);
        if (buf[0] != M_SD){ 
            printf("% s buf = %s\n", head, buf);
            buf[0]=0;
            //sleep(1); 
            continue;
        }
                
        if (buf[0] == M_SD){
            for (int i = 0; i < sizeof(struct_mes); i++)read(rx, ((void*)&mess_field)+i, 1);

            if (mess_field.AC == 1){
                for(int i=0;  ;i++){
                    read(rx, &buf[i], 1);
                    if (buf[i] == M_ED) {
                        buf[i] = 0;
                        break;
                    }
                }
                printf("%s Get message: %d  %lu  %lu  : %s \n", head, mess_field.AC, mess_field.DA, mess_field.SA, buf);
            }else{
                read(rx, &buf[0],3);
                if(buf[0]== M_ED) buf[0]=0;
                printf("%s Get MARKER: %d  %lu  %lu  : %s \n", head, mess_field.AC, mess_field.DA, mess_field.SA, buf);
                            
            }

            
        }

        switch(mess_field.AC){
            case  0: {
                if(self_id ==threads[0]){
                    flag = 1; break;
                }
                if(/*bloc== 0 &&*/ rand()%100 > 60){
                    mess_field.AC = 1;
                    number_thread = rand()%PC_NUM;
                    mess_field.DA = threads [number_thread];
                    mess_field.SA = self_id;
                    if (mess_field.DA == mess_field.SA) {
                        if(number_thread<PC_NUM-1){
                            mess_field.DA = threads [number_thread+1];
                        }else{
                            mess_field.DA = threads [number_thread-1];
                        }                        
                    }
                    memset(buf,0,1024);
                    for(int i = 0; i < 10; i++)buf[i] = 'a' + rand()%9;
                
                    printf("%s Create message: %s \n",head, buf);
                    //bloc = 1;
                }
                break;
            }

            case 1: {
                if (mess_field.DA == self_id){
                    if (strcmp(buf,"received")==0){
                        flag = 1;//если сообщение про получено адресатом и должно отбросится
                        printf("%s End of ring \n", head);

                        break;

                    }else{
                        printf("%s It's for ME: %s \n", head, buf);
                        mess_field.DA = mess_field.SA;
                        mess_field.SA = self_id;
                        strcpy(buf,"received");
                        break;
                    }
                    
                }
            
            }
        }
    
        if (flag != 1) {
            printf("%s Send message: %u  %lu  %lu  : %s \n", head, mess_field.AC, mess_field.DA, mess_field.SA, buf);
            sleep(1);
            write (tx,  &M_SD, 1);
            write(tx, &mess_field, sizeof(mess_field));
            write(tx, &buf[0], strlen(buf));
            write(tx, &M_ED, 1);
            
            flag = 0;
        }else{
            printf("%s Nothing for sent \n",head);
            sleep(1);
            flag = 0;
        }

        //flag = 0;
            
       

    }
};

int main(int argc, char const *argv[])
{
    int pipe_rx_tx [2];
    bool bb;
    printf("Количество компьютеров подключенных к сети: %d \n", PC_NUM);
    srand(time(NULL));
    
    //int chan [PC_NUM*2+1];

    for (int i = 0; i < PC_NUM; i++){

        if (pipe (pipe_rx_tx) == -1) {
            printf ("Error of pipe !\n");
            exit(1);
        }
        //fcntl(pipe_rx_tx[0], F_SETFL, O_BLO);
        chan[i*2] = pipe_rx_tx[0];
        chan[i*2+1] = pipe_rx_tx[1];
    }
    
    chan[2*PC_NUM]=chan[0];
            
    for (int i = 0; i<PC_NUM; i++) printf ("Pipe %u have RX : %u, TX : %u \n ", i, chan[i*2], chan[i*2+1]);

    for (int i = 0; i < PC_NUM; i++){
        if(pthread_create(&threads[i], NULL, pc, (void*)&chan[i*2+1]) == -1) {
            printf("Error pthread_create()\n");
            exit(1);
        } else {
            printf("Thread: %d | %lu | tx: %d | rx: %d\n", i, threads[i], chan[i*2+1], chan[i*2+2]);
        }
    }

    sleep(PC_NUM*2*6);

    for (int i = 0; i < PC_NUM+1; i++){
        if(pthread_cancel(threads[i]) == -1) {
            printf("Error pthread_cancel()\n");
            exit(1);
        }
    }

    for(int i=0; i<PC_NUM*2+1;i++) close(chan[i]);
}