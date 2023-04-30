#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdarg.h>
#include <stdbool.h>
#include <semaphore.h>

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>

#include <ctype.h>
#include <sys/types.h>

#include <sys/wait.h>

#include <sys/shm.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

FILE *file;

sem_t *out_mutex;
sem_t *customer_count_mutex;

sem_t *mutex_queue;
sem_t *queue1_postman;
sem_t *queue2_postman;
sem_t *queue3_postman;
sem_t *queue1_customer;
sem_t *queue2_customer;
sem_t *queue3_customer;

int *line_number;
bool *postoffice_open;
int *customer_count;

int *queue1;
int *queue2;
int *queue3;


int check_input(int argc,char *argv[]){
    // Nesprávný počet argumentů

    if(argc != 6){
        fprintf(stderr, "Error : Incorrect arg count(5 arguments are required)\n");
        return 1;
    }

    // Nesprávný argument

    for(int arg_num = 1; arg_num < 6; arg_num++)
    {

        char *curr_arg = argv[arg_num];
        for(int char_num = 0; curr_arg[char_num] != '\0'; char_num++)
        {

            char curr_char = curr_arg[char_num];
            if(!isdigit(curr_char)){
                fprintf(stderr, "Error : Incorrect argument(not a number)\n");
                return 1;
            }

        }
    }

    // 0 < TZ <= 10 000
    int TZ = atoi(argv[3]);
    if((TZ > 10000) || (0 > TZ)){
        fprintf(stderr,"Incorrect arg:TZ\n");
        return 1;
    }

    //  0 <= TU <= 100
    int TU = atoi(argv[4]);
    if(((0 > TU) || (TU > 100))){
        fprintf(stderr,"Incorrect arg:TU\n");
        return 1;
    }

    //  0 <= F <= 10000
    int F = atoi(argv[5]);
    if(((0 > F) || (F > 10000))){
        fprintf(stderr,"Incorrect arg:F\n");
        return 1;
    }

    return 0;
}

void mem_and_semaph_init(){
    // alokování sdílené paměti pro procesy
    line_number = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        *line_number = 1;
    out_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    postoffice_open = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *postoffice_open = true;
    mutex_queue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue1_postman = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue2_postman = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue3_postman = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue1_customer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue2_customer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue3_customer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    customer_count_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    queue1 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        *queue1 = 0;
    queue2 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        *queue2 = 0;
    queue3 = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        *queue3 = 0;

    // kontrola alokace
    if (    (line_number == MAP_FAILED)             ||
            (out_mutex == MAP_FAILED)               || 
            (postoffice_open == MAP_FAILED)         ||
            (mutex_queue == MAP_FAILED)             ||
            (queue1_postman == MAP_FAILED)          ||
            (queue2_postman == MAP_FAILED)          ||
            (queue3_postman == MAP_FAILED)          ||
            (queue1_customer == MAP_FAILED)         ||
            (queue1_customer == MAP_FAILED)         ||
            (queue2_customer == MAP_FAILED)         ||
            (queue3_customer == MAP_FAILED)         ||
            (customer_count_mutex == MAP_FAILED)    ||
            (queue1 == MAP_FAILED)                  ||
            (queue2 == MAP_FAILED)                  ||
            (queue3 == MAP_FAILED)                  ){
            exit(1);
    }
    
    // inicializace semaforů pro vlastní funkce
    sem_init(out_mutex, 1, 1);
    sem_init(customer_count_mutex, 1, 1);

    // inicializace semaforů řídících procesy
    sem_init(mutex_queue, 1, 1);
    sem_init(queue1_postman, 1, 0);
    sem_init(queue2_postman, 1, 0);
    sem_init(queue3_postman, 1, 0);
    sem_init(queue1_customer, 1, 0);
    sem_init(queue2_customer, 1, 0);
    sem_init(queue3_customer, 1, 0);
}

void mem_and_semaph_destroy(){
    // uvolnění alokované paměti
    sem_destroy(out_mutex);
    munmap(line_number,sizeof(int));
    munmap(out_mutex,sizeof(sem_t));

    munmap(postoffice_open,sizeof(bool));
    munmap(mutex_queue,sizeof(sem_t));
    munmap(queue1_postman, sizeof(sem_t));
    munmap(queue2_postman, sizeof(sem_t));
    munmap(queue3_postman, sizeof(sem_t));
    munmap(queue1_customer, sizeof(sem_t));
    munmap(queue2_customer, sizeof(sem_t));
    munmap(queue3_customer, sizeof(sem_t));
    munmap(customer_count_mutex,sizeof(sem_t));
    munmap(queue1,sizeof(int));
    munmap(queue2,sizeof(int));
    munmap(queue3,sizeof(int));
}

void my_print(const char * format, ...){
    // vlastní funkce my_print, která obsahuje flush bufferu a mutex
    sem_wait(out_mutex);
    va_list args;
    va_start (args, format);
    fprintf(file, "%d: ", *line_number);
    line_number[0]++;
    vfprintf(file, format, args);
    fflush(file);
    va_end(args);
    sem_post(out_mutex);
}

int random_number(int num) {
    srand(time(NULL));
    return rand() % num;
}

void change_customer_count(int *queue, int num){
    sem_wait(customer_count_mutex);
    *queue = *queue + num;
    sem_post(customer_count_mutex);
}



void servelastcustomers(int id){
    // funkce pro obsloužení posledních zákazníků ve frontě(úředník si nebere pauzy a neobsluhuje pouze jednoho v jednom loopu)
    while((*queue1 != 0) || (*queue2 != 0) || (*queue3 != 0)){
                        switch(1){
                                    case 1:
                                    sem_wait(mutex_queue);
                                        if((*queue1 > 0)){
                                            change_customer_count(queue1, -1);
                                            sem_post(mutex_queue);
                                            sem_post(queue1_postman);
                                            sem_wait(queue1_customer);
                                            my_print("U %d: serving a service of type 1\n", id);
                                            usleep((random_number(id+9)%11)*1000);
                                            my_print("U %d: service finished\n", id);
                                            break;
                                        }
                                        // fallthrough
                                    case 2:
                                        if((*queue2 > 0)){
                                            change_customer_count(queue2, -1);
                                            sem_post(mutex_queue);
                                            sem_post(queue2_postman);
                                            sem_wait(queue2_customer);
                                            my_print("U %d: serving a service of type 2\n", id);
                                            usleep((random_number(id+9)%11)*1000);
                                            my_print("U %d: service finished\n", id);
                                            break;
                                        }
                                        // fallthrough
                                    case 3:
                                        if((*queue3 > 0)){
                                            change_customer_count(queue3, -1);
                                            sem_post(mutex_queue);
                                            sem_post(queue3_postman);
                                            sem_wait(queue3_customer);
                                            my_print("U %d: serving a service of type 3\n", id);
                                            usleep((random_number(id+9)%11)*1000);
                                            my_print("U %d: service finished\n", id);
                                            break;
                                        }
                                        if((*queue1 == 0) || (*queue2 == 0) || (*queue3 == 0)){
                                            sem_post(mutex_queue);
                                            break;
                                        }
                        }
    }
}


void postmanspawn(int id, int TU){
    // funkce procesu úředník, začíná, pokud je pošta otevřená, tak obsluhuje zákazníky(pokud je 0 zákazníků, tak si bere pauzu), ve chvíli kdy je pošt zavřená, tak doobslouží zbytek a jde domů
    my_print("U %d: started\n", id);
    if(*postoffice_open == true){
            while(1){
                if(*postoffice_open == true){
                    if((*queue1 > 0) || (*queue2 > 0) || (*queue3 > 0)){
                        switch(1){
                            case 1:
                                sem_wait(mutex_queue);
                                if((*queue1 > 0)){
                                    change_customer_count(queue1, -1);
                                    sem_post(mutex_queue);
                                    sem_post(queue1_postman);
                                    sem_wait(queue1_customer);
                                    my_print("U %d: serving a service of type 1\n", id);
                                    usleep((random_number(id+9)%11)*1000);
                                    my_print("U %d: service finished\n", id);
                                    break;
                                }
                                // fallthrough  
                            case 2:
                                if((*queue2 > 0)){
                                    change_customer_count(queue2, -1);
                                    sem_post(mutex_queue);
                                    sem_post(queue2_postman);
                                    sem_wait(queue2_customer);
                                    my_print("U %d: serving a service of type 2\n", id);
                                    usleep((random_number(id+9)%11)*1000);
                                    my_print("U %d: service finished\n", id);
                                    break;
                                }
                                // fallthrough
                            case 3:
                                if((*queue3 > 0)){
                                    change_customer_count(queue3, -1);
                                    sem_post(mutex_queue);
                                    sem_post(queue3_postman);
                                    sem_wait(queue3_customer);
                                    my_print("U %d: serving a service of type 3\n", id);
                                    usleep((random_number(id+9)%11)*1000);
                                    my_print("U %d: service finished\n", id);
                                    break;
                                }
                                else{
                                    sem_post(mutex_queue);
                                    break;
                                }
                        }
                    } else {
                        if(*postoffice_open == true){
                            my_print("U %d: taking break\n", id);
                            if(TU != 0){
                                usleep((random_number(id+TU-1)%TU)*1000);
                            }
                            my_print("U %d: break finished\n", id);
                        }
                    }
                } else{
                    servelastcustomers(id);
                    my_print("U %d: going home\n", id);
                    break;
                }
            }
    } else{
        servelastcustomers(id);
        my_print("U %d: going home\n", id);
    }
}

void customerspawn(int id, int TZ){
    // funkce procesu zákazník, začíná, je uspána podle TZ, pokud je pošta otevřená, tak si vybere frontu ve které čeká na obsloužení úředníkem, poté odchází domů
    my_print("Z %d: started\n", id);
    if(TZ != 0){
    usleep((random_number(id+9999)%TZ)*1000);
    }
    if(*postoffice_open == false){
        my_print("Z %d: going home\n", id);
    }
    else{
        int service = random_number(id+2)%3 + 1;
        my_print("Z %d: entering office for a service %d\n", id, service);
        switch(service){
            case 1: 
                change_customer_count(queue1, 1);
                sem_post(queue1_customer);
                sem_wait(queue1_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep((random_number(id+9))*1000);
                my_print("Z %d: going home\n", id);
                break;
            case 2:
                change_customer_count(queue2, 1);
                sem_post(queue2_customer);
                sem_wait(queue2_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep((random_number(id+9))*1000);
                my_print("Z %d: going home\n", id);
                break;
            case 3:
                change_customer_count(queue3, 1);
                sem_post(queue3_customer);;
                sem_wait(queue3_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep((random_number(id+9))*1000);
                my_print("Z %d: going home\n", id);
                break;
        }
    }   
}



int main(int argc,char *argv[]){
    if(check_input(argc, argv) == 1){
        return 1;
    }
    int NZ = atoi(argv[1]); NZ = NZ + 0;    // počet zákazníků
    int NU = atoi(argv[2]); NU = NU + 0;    // počet úředníků
    int TZ = atoi(argv[3]); TZ = TZ + 0;    // doba kterou vytvořený zákazník čeká
    int TU = atoi(argv[4]); TU = TU + 0;    // doba přestávky úředníka
    int  F = atoi(argv[5]);  F =  F + 0;    // doba kterou je otevřena pošta


    if((file = fopen("proj2.out", "w")) == NULL){
        fprintf(stderr, "File did not open.");
        return 1;
    }
    

    mem_and_semaph_init();
    if(F == 0){
        *postoffice_open = false;
    }

    for(int i = 1; i <= NU; i++){               // vytváření úředníků
        pid_t id = fork();
        if(id == 0){
            postmanspawn(i, TU);
            exit(0);
        }
    }

    for(int i = 1; i <= NZ; i++){               // vytváření zákazníků
        pid_t id = fork();
        if(id == 0){
            customerspawn(i, TZ);
            exit(0);
        }
    }

    int open_time;
    if(F ==1){
        open_time = 1;
    }else if(F==0){
        open_time = 0;
    }else{
        open_time = (F/2) + (random_number(F)%(F/2));
    }

    if(F != 0){
        usleep((open_time)*1000);
    }
    my_print("closing\n");
    *postoffice_open = false;

    while(wait(NULL) > 0);
    mem_and_semaph_destroy();
    fclose(file);
    return 0;
}