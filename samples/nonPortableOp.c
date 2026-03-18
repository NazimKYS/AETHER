
#include <stdio.h>
#include <limits.h>


int main() {

// cast
short Sval1=0,Sval2=0;
unsigned short USval1=0,USval2=0;
int Ival1=0,Ival2=0;
unsigned int UIval1=0,UIval2=0;
long Lval1=0,Lval2=0;
unsigned long ULval1=0,ULval2=0;
long long res=0;
res=Sval1*Sval2+USval1*USval2+Ival1*Ival2+UIval1*UIval2+Lval1*Lval2+ULval1*ULval2;
res=Sval1*USval2+Ival1*UIval2+Lval1*ULval2;
res=USval1*Ival1+UIval2*Lval2;

if(Sval1%Sval2==0){
    printf("mod 0");

}else{
    printf("mod not 0");
}

    return 0;
}