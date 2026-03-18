// example-hw3.c
#include <limits.h>
#include <stdio.h>
//#include "example-hw3.h"

int static_var = 1;

int foo(int a) {
  static_var++;
  // if-else
  if (a == 1) {
    return 2;
  } else {
    return 4;
  }
}

int main() {
  int a = 0;
  int b;
  printf("a");
  if (a == 1 || a == 3) {
    //char data = CHAR_MAX;
    a = 2;
    b = 10;
  } else if (a == 2) {
  
  
    a = 4;
    b = 0;
    b = 100;
  } else {

    do {
      if (a == 4 || a == 8) {
        a = 2;
        b = a + 2 * b;
      } else {
        a = 4;
      }

      a += a;
      a = b + 3;
    } while (a < 100);
  }

  
}
