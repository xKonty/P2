#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int random_number(){
    srand(time(NULL));
    return rand()%3 + 1;
}

int main(){
    printf("%d\n", random_number());
}