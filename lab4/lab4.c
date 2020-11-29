#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

unsigned char sig = 0;

const char* st[7] =  {//образцы отправляемых сообщений
    "1message1",
    "2message2",
    "3message3",
    "4message4",
    "5message5",
    "6message6",
    "7mess7"
};

void* pcTread (void *param){
    char writeBuf[32] = "\0"; //отправляемое сообщение
    char* writePointer = writeBuf; //указатель на символ отправляемого сообщения
    char readBuf[36] = "\0"; //принимаемое сообщение
    char* readPointer = readBuf; //указатель на символ принимаемого сообщения
    int counCollizions = 0; //счётчик коллизий
    int flagSetColliz = 0;//флаг коллизий и счётчик отсрочки
    int coutZero = 0;
    unsigned long int ptid = pthread_self() % 10000;
    char threadId[32];

    while (true){
//To Do: увеличение задержки при попытке занять канал...
        sprintf(threadId, "%10u | %6u", time(NULL), ptid);
//        printf ("%s : sig = '%c' %X \n", threadId, sig, sig);
        switch (sig){//читаем символ из канала
        case 0://канал свободен
            
            
            if(strlen(readBuf)>0){//если что-то было получено, но пока об этом не отчитался
                printf("\n%s : Получено сообщение: %s \n", threadId, readBuf);//отчитаться
                memset(readBuf,0,36);//обнулить буфер сообщения
                readPointer = readBuf;//после отчёта восстанавливаем указатель на начало буфера
            }
            if (flagSetColliz == -1) flagSetColliz = 0;//снять флаг установки коллизии (отсрочки не касается)
            if (coutZero == 0) {coutZero=1; break;}else{//задерживаем "ноль", чтобы все увидели и отчитались
            if (coutZero == 1) {coutZero=2; break;}else{coutZero = 0; printf("%s : Catch ZERO \n", threadId);}}//первый ноль пропускаем, чтобы все увидели и отчитались
            if(*writePointer!= 0){
                printf("%s : Осталось пока неотосланным \n", threadId);//отчитаться, у кого сбой с отправкой
            }
            
            
            if (flagSetColliz == 0 && (rand()%100 <60) && *writePointer==0){//вероятность необходимости отправить сообщение
                int i = rand()%(sizeof(st)/sizeof(st[0]));//выборка сообщения из образцов
                memcpy(writeBuf, st[i],strlen(st[i])+1);//перенос сообщения в буфер
                
                printf("%s : Готовим новое сообщение: %s \n", threadId, writeBuf);//отчитаться
            }
            writePointer = writeBuf;//указатель на начало сообщения
            if(*writePointer!= 0){
                sig = 0xFF; //сообщаем о необходимости занять канал
                printf("%s : Занимаю канал \n", threadId);
            }
            break;
        case 0xFF://кто-то занимает канал...
            if (*writePointer){//если канал заняли мы...
                sig = *writePointer;//отправляем символ
                
            }
            break;
        case 0xFE://сообщение о коллизии
            memset(readBuf,0,36);//очистить буфер принятого сообщенияш
            readPointer = readBuf;//указатель принятого сообщения на начало
            writePointer = writeBuf;//указатель отправляемого сообщения на начало
            
            printf("%s : got jam, claring read buffer\n", threadId);//

            if(flagSetColliz == -1) {//если коллизию установил этот тред
                sig = 0; //снимаем сигнал коллизии
                
                flagSetColliz = rand()%(1+counCollizions*8); // rand генерируем отсрочку
                printf("%s : Снимаю коллизию, обнуляю канал: %d (wait %d ticks)\n", threadId, counCollizions, flagSetColliz);
            }
            
            break;
        
        default://в канале какой-то символ...
            if(*writePointer && flagSetColliz == 0){//если мы безотлогательно пишем в канал...
                    if(sig == *writePointer){//если символ совпал с отосланным
                        *readPointer = sig;//сохранаем в буфер принятого..
                        readPointer++;//указатель на следующий символ
                        writePointer++;//указатель на следующий символ
                        sig = *writePointer;//следующий символ отправить в канал
                        if (*writePointer==0) {//если символ "конца строки"(сообщение отправлено полностью)
                            counCollizions = 0; //сбросить счётчик коллизий
                            memset(writeBuf,0,32); //сбросить буфер сообщения
                            writePointer = writeBuf;//указатель на начало
                        }
                    }else{//если отправляем сообщение, но...
                        sig = 0xFE;//если символ не совпал с отправленным, отправить сигнал коллизии
                        printf("%s : Отпрален сигнал \"коллизия\": %d\n", threadId, counCollizions+1);
                        counCollizions++;//нарастить счётчик коллизий
                        flagSetColliz = -1;//снять коллизию должен тот, кто установил, чтобы все успели прочитать
                        writePointer = writeBuf;//перейти на начало отправляемого сообщения
                        if(counCollizions==10){//если счётчик коллизий зашкалил...
                            printf("%s : Уведомление о невозможности отправить сообщение!\n", threadId);
                            counCollizions = 0;//обнуляем счётчик
                            memset (writeBuf,0,32);//обнуляем сообщение
                        }
                    }
                
            }else{//если пишем НЕ мы...
                    *readPointer = sig;//сохраняем символ в буфер
                    readPointer++;//переходим на следующую позицию
                }
            break;
        } 

        if(flagSetColliz > 0) {
            flagSetColliz--;//если отсрочка, то декрементируем...
//            printf("%s : Осталось циклов ожидания %d\n", threadId, flagSetColliz);
        }

        usleep(500000);//засыпаем... пусть работают другие... (usleep(мкс),sleep(сек))
        
    }
}

int main(int argc, char const *argv[])
{
    srand(time(NULL));
  //  for (int i =0; i<10; i++) printf("%d ", rand()%((1+i)*8));
    pthread_t* threadList=0;//объявляем указатель на список дочерних потоков
    int iThread=0;//заводим индекс для списка дочерних потоков
 
    printf("Введите количество подключенных к каналу устройств: \t");
    scanf("%d", &iThread);
    printf("\n");
    threadList = (pthread_t*)malloc((iThread)*sizeof(pthread_t));//запрашиваем у системы память под массив ID дочерних потоков
    if(!threadList){ //если система память выделить не может,то..
        printf("\r\nmalloc failed, cannot create thread\r\n");//выводим сообщение об ограничении памяти и невозможности породить дочерние потоки
        exit(1);
    }
    pthread_t nC = 0;//заводим переменную под ID потока
    for (int i = 0; i<iThread; i++){
        if(pthread_create(&nC, NULL, pcTread, NULL) != 0) {//порождаем дочерний поток, скармливая адрес на память для ID и начала функции потока
            printf("Thread fail!\n");//если не получилось, сообщаем об этом
            exit(1);
        }
        threadList[i] = nC;//внесём в список дочерних потоков новый порождённый для учёта и контроля
    }
    sleep(180);
    for (int i = 0; i<iThread; i++)
        pthread_cancel(threadList[i]);//прибиваем дочерний процесс
    free(threadList);//освобождаем память из-под списка детёнышей
    return 0;
}
