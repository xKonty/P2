#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <stdarg.h>
#include <stdbool.h>
#include <semaphore.h>

#include <unistd.h>
#include <string.h>



#include <ctype.h>
#include <time.h>
#include <sys/types.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

FILE *file;

sem_t *out_mutex;
sem_t *customer;
sem_t *postman;
sem_t *mutex_queue;

int *line_number;
bool *postoffice_open;


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
    line_number = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    line_number[0] = 1;
    out_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    postoffice_open = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    customer = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    postman = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    mutex_queue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (out_mutex == MAP_FAILED) {//TODO:přidat podmínky
        exit(1);
    }
    
    sem_init(out_mutex, 1, 1);

    sem_init(customer, 1, 0);
    sem_init(postman, 1, 0);
    sem_init(mutex_queue, 1, 1);
}

void mem_and_semaph_destroy(){
    sem_destroy(out_mutex);
    munmap(line_number, sizeof(int));
    munmap(out_mutex, sizeof(sem_t));

    munmap(postoffice_open, sizeof(bool));
    munmap(customer,sizeof(sem_t));
    munmap(postman,sizeof(sem_t));
    munmap(mutex_queue,sizeof(sem_t));
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



void postofficespawn(){
    my_print("post office open\n");
}

void postmanspawn(int id){ 
    my_print("U %d: started\n", id);
    sem_wait(customer);
    sem_post(postman);
}

void customerspawn(int id, int TZ){
    my_print("Z %d: started\n", id);//TODO: po uzavření pošty Z nechodí
    usleep(random_number(id)%TZ);

    if()
    int service = random_number(id);
    my_print("Z %d: entering office for a service %d\n", id, service);
    sem_wait(mutex_queue);
    sem_post(customer);
    sem_wait(postman);
    my_print("Z %d: called by office worker\n", id);
    sem_post(mutex_queue);
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
            postmanspawn(i);
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

    pid_t id = fork();
    if(id == 0){
        postofficespawn();
        exit(0);
    }

    while(wait(NULL) > 0);
    mem_and_semaph_destroy();
    fclose(file);
    return 0;
}