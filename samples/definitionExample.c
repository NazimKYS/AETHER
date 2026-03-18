#include "nazim.h"
//#include <limits.h>
//#include <stddef.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <string.h>
//#include <time.h>

int getUserId(char username[], char password[]) { return 100; }
void read(int *serviceId) { serviceId = 6; }
void readAndWriteService(int a, int b) {}
void readOnlyService(int a, int b) {}

void foo(char username[], char password[]) {
  int userId = getUserId(username, password); userId=0;
  // 0 <= userId <= 150*10^6
  int serviceId=99,a=0,b=0,c=0;
  serviceId=-60;a=-100;b=-200;c=-1000;//serviceId=64;// read(&serviceId); 1 <= serviceId <= 64
  long int uid_sid;
  uid_sid = (long)userId * serviceId;
  if (uid_sid == 0) {      serviceId = 0;
    readAndWriteService(uid_sid, serviceId);
    if(serviceId>35 ){ //  && <==> and 
        readOnly(uid_sid, serviceId); serviceId=60;
    }else{b=18;  serviceId=15;
      readAndWriteService(uid_sid, serviceId);
    }
  } else if(  userId<0  && (serviceId !=0 || userId>0)  ) {a=15;
    readOnlyService(uid_sid, serviceId);
  }else{c=a*(b+serviceId);
    writeOnly(uid_sid, serviceId);
  }
  
  
  
  return;
}

