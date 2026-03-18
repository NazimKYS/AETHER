#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int random_int(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    return rand() % (max - min + 1) + min;
}

int getUserId(char username[], char password[]) { return 100; }
void readAndWriteService(int a, int b) {}
void readOnlyService(int a, int b) {}
int getServiceId(){}
void foo(char username[], char password[]){
    int userId= random_int(0,150000000);
    int serviceId= random_int(1,64);
    long int uid_sid=(long)userId*serviceId;
    if (uid_sid==0){
        readAndWriteService(uid_sid,serviceId);
    }else{
        readOnlyService(uid_sid,serviceId);
    }
    return;
}
