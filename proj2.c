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

sem_t *mutex_queue1;
sem_t *mutex_queue2;
sem_t *mutex_queue3;
sem_t *queue1_postman;
sem_t *queue2_postman;
sem_t *queue3_postman;
sem_t *queue1_customer;
sem_t *queue2_customer;
sem_t *queue3_customer;
sem_t *customer_count_mutex;

int *line_number;
bool *postoffice_open;
int *customer_count;

int *queue1;
int *queue2;
int *queue3;


int check_input(int argc,char *argv[]){
    //#remove
    return 0;

    // INCORRECT ARG COUNT

    if(argc != 6){
        fprintf(stderr, "Error : Incorrect arg count(5 arguments are required)\n");
        return 1;
    }

    // INCORRECT ARG

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
    int TZ = atoi(argv[4]);
    if(!(TZ <= 10000 && 0 < TZ)){
        fprintf(stderr,"Incorrect arg:TZ\n");
        return 1;
    }

    //  0 <= TU <= 100
    int TU = atoi(argv[5]);
    if(!(0 <= TU && TU <= 100)){
        fprintf(stderr,"Incorrect arg:TU\n");
        return 1;
    }

    //  0 <= F <= 10000
    int F = atoi(argv[6]);
    if(!(0 <= F && F <= 10000)){
        fprintf(stderr,"Incorrect arg:F\n");
        return 1;
    }

    return 0;
}

void mem_and_semaph_init(){
    line_number = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        *line_number = 1;
    out_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    postoffice_open = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *postoffice_open = true;
    mutex_queue1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    mutex_queue2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    mutex_queue3 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
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


    if (out_mutex == MAP_FAILED) {//TODO:přidat podmínky
        exit(1);
    }
    
    sem_init(out_mutex, 1, 1);


    sem_init(mutex_queue1, 1, 1);
    sem_init(mutex_queue2, 1, 1);
    sem_init(mutex_queue3, 1, 1);
    sem_init(queue1_postman, 1, 0);
    sem_init(queue2_postman, 1, 0);
    sem_init(queue3_postman, 1, 0);
    sem_init(queue1_customer, 1, 0);
    sem_init(queue2_customer, 1, 0);
    sem_init(queue3_customer, 1, 0);
    sem_init(customer_count_mutex, 1, 1);
}

void mem_and_semaph_destroy(){
    sem_destroy(out_mutex);
    munmap(line_number,sizeof(int));
    munmap(out_mutex,sizeof(sem_t));

    munmap(postoffice_open,sizeof(bool));
    munmap(mutex_queue1,sizeof(sem_t));
    munmap(mutex_queue2,sizeof(sem_t));
    munmap(mutex_queue3,sizeof(sem_t));
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
    *queue = num;
    sem_post(customer_count_mutex);
}




void postmanspawn(int id, int TU){ 
    my_print("U %d: started\n", id);
    if(*postoffice_open == true){
            while(*postoffice_open == true){
                if((*queue1 == 1) || (*queue2 == 1) || (*queue3 == 1)){
                    switch(1){
                        case 1:
                            if((*queue1 == 1)){
                        ;       sem_wait(queue1_customer);
                                sem_post(queue1_postman);
                                my_print("U %d: serving a service of type 1\n", id);
                                change_customer_count(queue1, 0);
                                usleep(random_number(id+9)%11);
                                my_print("U %d: service finished\n", id);
                                break;
                            }
                            // fallthrough
                        case 2:
                            if((*queue2 == 1)){
                                sem_wait(queue2_customer);
                                sem_post(queue2_postman);
                                my_print("U %d: serving a service of type 2\n", id);
                                change_customer_count(queue2, 0);
                                usleep(random_number(id+9)%11);
                                my_print("U %d: service finished\n", id);
                                break;
                            }
                            // fallthrough
                        case 3:
                            if((*queue3 == 1)){
                                sem_wait(queue3_customer);
                                sem_post(queue3_postman);
                                my_print("U %d: serving a service of type 3\n", id);
                                change_customer_count(queue3, 0);
                                usleep(random_number(id+9)%11);
                                my_print("U %d: service finished\n", id);
                                break;
                            }
                            else{
                                break;
                            }
                    }
                } else {
                    my_print("U %d: taking a break\n", id);
                    usleep(random_number(id+TU-1)%TU);
                    my_print("U %d: break finished\n", id);
                }
            }
    }
}

void customerspawn(int id, int TZ){
    my_print("Z %d: started\n", id);//TODO: po uzavření pošty Z nechodí
    usleep(random_number(id+9999)%TZ);
    if(postoffice_open == false){
        my_print("Z %d: going home\n", id);
    }
    else{
        int service = random_number(id+2)%3 + 1;
        my_print("Z %d: entering office for a service %d\n", id, service);
        switch(service){
            case 1: 
                sem_wait(mutex_queue1);
                change_customer_count(queue1, 1);
                sem_post(queue1_customer);
                sem_wait(queue1_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep(random_number(id+9));
                my_print("Z %d: going home\n", id);
                sem_post(mutex_queue1);
                break;
            case 2:
                sem_wait(mutex_queue2);
                change_customer_count(queue2, 1);
                sem_post(queue2_customer);
                sem_wait(queue2_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep(random_number(id+9));
                my_print("Z %d: going home\n", id);
                sem_post(mutex_queue2);
                break;
            case 3:
                sem_wait(mutex_queue3);
                change_customer_count(queue3, 1);
                sem_post(queue3_customer);
                sem_wait(queue3_postman);
                my_print("Z %d: called by office worker\n", id);
                usleep(random_number(id+9));
                my_print("Z %d: going home\n", id);
                sem_post(mutex_queue3);
                break;
        }
    }   
}

void postofficespawn(int F){
usleep(F);
}

int main(int argc,char *argv[]){
    check_input(argc, argv);
    int NZ = atoi(argv[1]); NZ = NZ + 0;    // počet zákazníků
    int NU = atoi(argv[2]); NU = NU + 0;    // počet úředníků
    int TZ = atoi(argv[3]); TZ = TZ + 0;    // doba kterou vytvořený zákazník čeká
    int TU = atoi(argv[4]); TU = TU + 0;    // doba přestávky úředníka
    int  F = atoi(argv[5]);  F =  F + 0;    // doba kterou je otevřena pošta
    mem_and_semaph_init();

    if((file = fopen("proj2.out", "w")) == NULL){
        fprintf(stderr, "File did not open.");
        return 1;
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

    while(wait(NULL) > 0);
    mem_and_semaph_destroy();
    fclose(file);
    return 0;
}