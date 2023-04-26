#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>

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

int *line_number;


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

void semaph_init(){
    line_number = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    line_number[0] = 1;
    srand(time(NULL));

    out_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (out_mutex == MAP_FAILED) {
        exit(1);
    }
    
    sem_init(out_mutex, 1, 1);
}

void semaph_destroy(){
    sem_destroy(out_mutex);
    munmap(out_mutex, sizeof(sem_t));
    munmap(line_number, sizeof(int));
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





void postman(int id){ 
    my_print("U %d: started\n", id);
    
}

void customer(int id){
    my_print("Z %d: started\n", id);//TODO: po uzavření pošty Z nechodí
    int service = id%3;//TODO: náhodná čísla
    my_print("Z %d: entering office for a service %d\n", id, service);
    
}

int main(int argc,char *argv[]){
    check_input(argc, argv);
    int NZ = atoi(argv[1]); NZ = NZ + 0;
    int NU = atoi(argv[2]); NU = NU + 0;
    int TZ = atoi(argv[3]); TZ = TZ + 0;
    int TU = atoi(argv[4]); TU = TU + 0;    
    int  F = atoi(argv[5]);  F =  F + 0;
    semaph_init();

    if((file = fopen("proj2.out", "w")) == NULL){
        fprintf(stderr, "File did not open.");
        return 1;
    }

    for(int i = 1; i <= NU; i++){               // vytváření úředníků
        pid_t id = fork();
        if(id == 0){
            postman(i);
            exit(0);
        }
    }

    for(int i = 1; i <= NZ; i++){               // vytváření zákazníků
        pid_t id = fork();
        if(id == 0){
            customer(i);
            exit(0);
        }
    }

    while(wait(NULL) > 0);
    semaph_destroy();
    fclose(file);
    return 0;
}