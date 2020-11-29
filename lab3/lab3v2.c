#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

typedef struct //структура для передачи "заголовочной информации" раскодировщику
    {
        int bitHamming; //количество проверочных битов Хемминга
        int lenSrcPkgBit;//размер исходного пакета в битах
        int lenSrcPkgByte;//размер исходного пакета в байтах
        int lenResPkgBit;//длина закодированного пакета в битах
        int lenResPkgByte; //длина закодированного пакета в байтах
        int quantPkgs;//количество пакетов на которое разделено сообщение


    } hammingData;


char* parser (hammingData* base , char* message)
{
    (*base).lenSrcPkgBit = (1<< ((*base).bitHamming-1));
    (*base).lenResPkgBit = (*base).lenSrcPkgBit + (*base).bitHamming;
    (*base).lenSrcPkgByte = (*base).lenSrcPkgBit >> 3;
    (*base).lenResPkgByte = ((base->lenSrcPkgBit + (*base).bitHamming) >> 3) + ( ((*base).lenSrcPkgBit + (*base).bitHamming)&7 == 0 ? 0 : 1 );
    (*base).quantPkgs = strlen(message) / (*base).lenSrcPkgByte;
    (*base).quantPkgs += ((strlen(message))&((*base).lenSrcPkgByte-1)?1:0);
    char* encodedPackages = (char*)malloc( ((*base).quantPkgs*(*base).lenResPkgByte) );
    if (encodedPackages==NULL)
    { 
        printf("Memory allocation ERROR ! \n");
    }else{
    memset(encodedPackages,0,((*base).quantPkgs*(*base).lenResPkgByte));

        for (int i = 0; i< (*base).quantPkgs; i++)
        {
             encode (base, encodedPackages+(i*(*base).lenResPkgByte), message+(i*(*base).lenSrcPkgByte));
        }
    }
    return encodedPackages;
}

void encode (hammingData* base, char* cod, char* mes) //функция кодирования пакета
{
    int bitChet = (1<< ((*base).bitHamming - 1))-1;
    int bitIndSrc = (*base).lenSrcPkgBit- 1;
    for (int i = (*base).lenResPkgBit-1; i>=0; i--)
    {
        if ((i+1)==(bitChet+1)){
            char conBit = 0;
            for(int j = (*base).lenResPkgBit-1; j>i; j--){
                if ((j+1)&(i+1)){
                    conBit ^= (cod[j>>3] & (1<<(j&7)) )>>(j&7);
                }
            }
            cod[i>>3] |= (conBit<<(i&7));
            bitChet = bitChet >> 1;
        }else{ 
                cod[i>>3] |= ((mes[bitIndSrc>>3]&(1<<(bitIndSrc&7)))>>(bitIndSrc&7))<<(i&7);
                bitIndSrc--;
        }
    }
    printf("Изначальный пакет:");
    for (int i = 0; i< (*base).lenSrcPkgBit; i++)
    {
        if ((i&7) == 0) printf("\t");
        printf("%d", (mes[i>>3] & (1<<(i&7)))?1:0 );
    }
    printf("\nПолученный пакет: ");
    for (int i = 0; i< (*base).lenResPkgBit; i++)
    {
        if ((i&7)==0) printf("\t");
        printf("%d", (cod[i>>3] & (1<<(i&7)))?1:0 );             
    }
    printf("\n");
    return;
}

void chanell (void) //эмуляция помех в канале
{

}

void rider (void) //сборка сообщения из пакетов
{

}

void decode (void) //раскодирование пакета
{

}

int main(int argc, char const *argv[])
{
    hammingData baseData;
    char enterMessage[128];
    char* massPackags;
    printf("Enter quantity of Hamming:"); 
        scanf("%d", &baseData.bitHamming);
    while(true) {
        memset(enterMessage,0,128);
        printf("Enter massege or ''q'' for exit:\n");
        scanf("%s", enterMessage);   
        if (strncasecmp(enterMessage, "q", 2) == 0) {
            break;
        }
        massPackags = parser(&baseData, enterMessage);
        








        free(massPackags)

    ;}
    return 0;
}