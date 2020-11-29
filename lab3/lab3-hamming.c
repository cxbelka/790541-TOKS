#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

 typedef struct //структура для передачи "заголовочной информации" раскодировщику
    {
        int bitHamming; //количество проверочных битов Хемминга
        int lenSrcPkgBit;//размер исходного пакета в битах
        int lenSrcPkgByte;//размер исходного пакета в байтах
        int lenResPkgBit;//длина закодированного пакета в битах
        int lenResPkgByte; //длина закодированного пакета в байтах
        int quantPkgs;//количество пакетов на которое разделено сообщение


    } hammingData; 

//функция разбивающая сообщение на пакеты
char* parser (hammingData* base , char* message) //base - структура "заголовочной информации", message - указатель на введённое сообщение
{
    //рассчитать размер пакета по количеству доп.битов
    //поскольку мы имеем дело со степенями двойки, то вместо целочисленной арифметики, максимально будем использовать побитовые операции сдвигов и т.п.
       
    (*base).lenSrcPkgBit = (1<< ((*base).bitHamming-1));//без учёта длины кода Хеминга, вместо возведения 2 в степень n-1
    
    (*base).lenResPkgBit = (*base).lenSrcPkgBit + (*base).bitHamming; //заодно посчитаем длину кодированного пакета в битах, пригодится...

    //printf ("Size of segment (бит): %d \n", (*base).lenSrcPkgBit);//выводим для проверки
    (*base).lenSrcPkgByte = (*base).lenSrcPkgBit >> 3;//вместо деления на 8 количество бит переводим в байты
    
    //вычислить колчество пакетов
   
    //поскольку количество байт в пакете может быть и нечётным, то выясним, понадобиться ли дополнительный бит
    //и чтобы избежать операции остатка от деления, реализуем через битовую операцию амперсанта
    //зная, что коды Хеминга кодируют количество бит(байт) в пакете кратное степеням двойки
    (*base).quantPkgs = strlen(message) / (*base).lenSrcPkgByte;
    (*base).quantPkgs += ((strlen(message))&((*base).lenSrcPkgByte-1)?1:0);//вместо деления проверяем на наличие единичных битов в хвосте числа
    //printf("Quantity all (pack) = %d \n", (*base).quantPkgs);
    
    //сколько байт займёт уже кодированный пакет?
    //важно заметить, что мы будем проверять сразу размер результирующего слова, а не исходного
    //поэтому при 9 и более кодовых бит Хеминга, условие будет выполняться, ввиду того, что 8 от 9 даст поправку в первом сдвиге, а оставшаяся 1 в тернарной операции
    (*base).lenResPkgByte = ((base->lenSrcPkgBit + (*base).bitHamming+1) >> 3) + ( ((*base).lenSrcPkgBit + (*base).bitHamming+1)&7 == 0 ? 0 : 1 );
    //printf ("Size of resulte package (byte): %d \n", (*base).lenResPkgByte);//выводим для проверки

    //выделить память для пакетов зная размерность массива
    char* encodedPackages = (char*)malloc( ((*base).quantPkgs*(*base).lenResPkgByte) );
    if (encodedPackages==NULL)
    { 
        printf("Memory allocation ERROR ! \n");
    } else {
        //printf("Memory is available. Address = %d \n", encodedPackages);
    };
    memset(encodedPackages,0,((*base).quantPkgs*(*base).lenResPkgByte));
    //разделить сообщения на пакеты с вызовом функции кодирования
    char buf [30];

        for (int i = 0; i< (*base).quantPkgs; i++)
        {
            memcpy(buf, message+(i*(*base).lenSrcPkgByte),(*base).lenSrcPkgByte); //копируем кусок сообщения в буфер для передачи кодировщику пакета
            printf ("Part %d of mass: %s \n", i, buf);
            printf("Приступаем к кодированию %d пакета... \n", i);
            encode (base, encodedPackages+(i*(*base).lenResPkgByte), message+(i*(*base).lenSrcPkgByte));
        }

    return encodedPackages;
}

void encode (hammingData* base, char* cod, char* mes) //функция кодирования пакета
{
    
    //здесь есть два варианта... можно пойти в прямом порядке, 
    //но тогда мы заранее не знаем какие биты пакета какие плейсхолдеры займут, чтобы заранее посчитать вставляемый бит чётности
    //можно пойти в обратном порядке, но вопрос количеству вложенных циклов, которые нужно будет дополнительно считать...
    //сча проверим...
    //стараемся всё писать битовыми операциями...
    //заводим переменную для индекса ближайшего бита чётности, начиная с конца
    //далее нужно учесть машинный счёт битов [15..0], вместо [16..1]
    int bitChet = (1<< ((*base).bitHamming - 1))-1 ; //-1 это поправка на машинную нумерацию
    //заведём теременную для изменения индекса в исходном пакете сообщения
    int bitIndSrc = (*base).lenSrcPkgBit- 1;
    int chetAll = 0;
    //шагаем от конца кодированного слова к началу...
    for (int i = (*base).lenResPkgBit-1; i>=0; i--)
    {
        
        //сравниваем индекс текущего бита, равен ли он позиции бита чётности...
        if ((i+1)==(bitChet+1)){ //если рабочую позицию должен занять бит чётности, то высчитываем его...
            //printf("i+1 = %d -- является битом чётности\n", i+1);
            char conBit = 0; //переменная для подсчёта чётности в наборе битов
            for(int j = (*base).lenResPkgBit-1; j>i; j--){
                //формула учёта значения бита в бите чётности
                if ((j+1)&(i+1)){//если индекс бита соответствует контролируемому битом чётности
                    
                    conBit ^= (cod[j>>3] & (1<<(j&7)) )>>(j&7);

                }

                //printf("j+1 = %d, conBit = %d \t", j+1, conBit);
            }
            cod[i>>3] |= (conBit<<(i&7));
            bitChet = bitChet >> 1;
            //printf("\n bitChet = bitChet >> 1 и равен %d;\n", bitChet);
        }else{ //если это не бит чётности, то просто переносим бит из исходных данных
                //printf("i = %d, bitIndSrc = %d \t", i, bitIndSrc);
                cod[i>>3] |= ((mes[bitIndSrc>>3]&(1<<(bitIndSrc&7)))>>(bitIndSrc&7))<<(i&7);
                bitIndSrc--;
        }
        chetAll ^= (cod[i>>3] & (1<<(i&7)) )>>(i&7);//заодно подсчитываем общую чётность

        //выбираем байт сдвига от начала рабочего участка в обратном порядке

    }

    cod[(*base).lenResPkgBit>>3] |= (chetAll<<((*base).lenResPkgBit&7));//вставляем общий бит чётности в конец пакета

    //выводим изначальный пакет побитово
    printf(" Изначальный пакет: ");
    for (int i = 0; i< (*base).lenSrcPkgBit; i++)
    {
        if ((i&7) == 0) printf("\t");

        printf("%d", (mes[i>>3] & (1<<(i&7)))?1:0 );
              
    }
    printf("\nКодированный пакет: ");
    for (int i = 0; i<= (*base).lenResPkgBit; i++)
    {
        if ((i&7)==0) printf("\t");

        printf("%d", (cod[i>>3] & (1<<(i&7)))?1:0 );
              
    }
    printf("\n");
    
    return;
}

void chanell (hammingData* base, char* mess) //эмуляция помех в канале
{
    int mist; //количество ошибок в канале
    int i = 0, ind;
    int len = (*base).quantPkgs*(*base).lenResPkgBit;
    printf("Enter number of errors in the channel:");
    scanf("%d", &mist); //вводим количество ошибок

    while (i<mist)
    {
        ind = rand()%len;
        mess[ind>>3] ^= (1<<(ind&7));
        i++;     
    }
    
}

char* rider (hammingData* baseInf, char* code) //сборка сообщения из пакетов
{
    char* decodedPackages = (char*)malloc( ((*baseInf).quantPkgs*(*baseInf).lenSrcPkgByte) );
    if (decodedPackages==NULL)
    { 
        printf("Memory allocation ERROR ! \n");
    } else {
       
    memset(decodedPackages,0,((*baseInf).quantPkgs*(*baseInf).lenResPkgByte));

    for (int i = 0; i< (*baseInf).quantPkgs; i++)
        {
            //memcpy(buf, message+(i*(*base).lenSrcPkgByte),(*base).lenSrcPkgByte); //копируем кусок сообщения в буфер для передачи кодировщику пакета
            printf ("\nPart %d of mass: \n", i);
            decode(baseInf, code+(i*(*baseInf).lenResPkgByte), decodedPackages+(i*(*baseInf).lenSrcPkgByte));
        }

    
    }
    return decodedPackages;
}

void decode (hammingData* base, char* cod, char* mes) //раскодирование пакета
{
    int control = 0; //для рассчёта кода Хеминга в полученном сообщении
    int real = 0; //для выборки переданного кода Хеминга
    int chetAll = 0;//для дополнительного бита чётности
    int bitChet = (1<< ((*base).bitHamming - 1))-1 ;//позиция крайнего бита чётности уменьшается сдвигом
    int posChet = (*base).bitHamming-1; //позиция в real куда писать выбранную кодовую цифру уменьшается инкриментом
    int bitIndSrc = (*base).lenSrcPkgBit- 1; //индексы итогового сообщения
    
    printf("\nПолученный пакет: ");
    for (int i = 0; i<= (*base).lenResPkgBit; i++)
    {
        if ((i&7)==0) printf("\t");

        printf("%d", (cod[i>>3] & (1<<(i&7)))?1:0 );
              
    }
    printf("\n");

    for (int i = (*base).lenResPkgBit-1; i>=0; i--)
    {   
        chetAll ^= (cod[i>>3] & (1<<(i&7)) )>>(i&7);
        //printf ("chetAll = %d \t", chetAll);
        if ((i+1)==(bitChet+1)){ //если рабочую позицию должен занять бит чётности, то высчитываем его...
            real |= (cod[i>>3] & (1<<(i&7)) )>>(i&7)<<posChet ;
            //printf("real: %x \t", real);
            char conBit = 0; //переменная для подсчёта чётности в наборе битов
            for(int j = (*base).lenResPkgBit-1; j>i; j--){
                //формула учёта значения бита в бите чётности
                if ((j+1)&(i+1)){//если индекс бита соответствует контролируемому битом чётности
                    
                    conBit ^= (cod[j>>3] & (1<<(j&7)) )>>(j&7);

                }

                }
            control |= (conBit<<posChet);
            //printf("control: %x \n", control);
            posChet--;
            bitChet = bitChet >> 1;
            //printf("\n bitChet = bitChet >> 1 и равен %d;\n", bitChet);
        }
    
    }
    int ind = control^real;
       
    int usl = chetAll ^ ((cod[(*base).lenResPkgBit>>3] & (1 << ((*base).lenResPkgBit&7))) >> ((*base).lenResPkgBit&7));

    printf ("\n control XOR real = %d, chetAll = %d, было = %d" , control^real, chetAll, (cod[(*base).lenResPkgBit>>3] & (1 << ((*base).lenResPkgBit&7)) >> ((*base).lenResPkgBit&7)));

    if (ind <= (*base).lenResPkgBit && ind>0 && usl!=0)
    {
        ind--;
        cod[ind>>3] ^= (1<<(ind&7));
        printf ("\nОбнаружена одиночная ошибка,.. исправляем...");
    }else
    {
        if (ind && !usl) 
        printf("\nОбнаружена двойная ошибка... исправить не можем");
        
    }
    

    bitChet = (1<< ((*base).bitHamming - 1))-1 ;//опять загружаем начальное положение бита чётности

    for (int i = (*base).lenResPkgBit-1; i>=0; i--)
    {
        if ((i+1)==(bitChet+1)){ //если рабочую позицию должен занять бит чётности, то высчитываем его...
                        
            bitChet = bitChet >> 1;
            continue;
            //printf("\n bitChet = bitChet >> 1 и равен %d;\n", bitChet);
        }else{ //если это не бит чётности, то просто переносим бит из исходных данных
                //printf("i = %d, bitIndSrc = %d \t", i, bitIndSrc);
                mes[bitIndSrc>>3] |= ((cod[i>>3]&(1<<(i&7)))>>(i&7))<<(bitIndSrc&7);
                bitIndSrc--;
        }
    }
    printf ("\t Исходное сообщение %s : ", mes);
}

int main(int argc, char const *argv[])
{
   
    hammingData baseData; //в этой переменной храняться "заголовочные данные" для раскодирования

    char enterMessage[128]; //буфер сообщения
    char* massPackags; //указатель на массив подготовленных к отправке пакетов
    int sizeMessage;
    char* endMessage;
        
    srand(time(NULL));
    
    printf("Enter quantity of Hamming:"); 
        scanf("%d", &baseData.bitHamming); //вводим число бит Хемминга
    
    while(true) {
        memset(enterMessage,0,128);
        printf("\nEnter massege or ''q'' for exit:\n"); //сообщаем о необходимости ввести сообщение
        scanf("%s", enterMessage); //принимаем на вход сообщение
        
        if (strncasecmp(enterMessage, "q", 2) == 0) { //если введён 'q'...
            break; //покидаем цикл и выходим из программы
        }
        sizeMessage = strlen(enterMessage);
        printf("Volume message (byte): %d \n", sizeMessage);

        massPackags = parser(&baseData, enterMessage);
        printf("Volume sizePakage (bit): %d \n", baseData.lenSrcPkgBit);
        
        printf("MassPackage (address) = %d \n", massPackags);

        chanell(&baseData, massPackags);

        endMessage = rider(&baseData, massPackags);



        free(massPackags)

    ;}
    return 0;
}

