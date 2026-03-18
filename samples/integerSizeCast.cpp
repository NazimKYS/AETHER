/*#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>*/
#include <iostream>

int main(int argc, char** argv) {

	int a = 134217728;
	int b = 64;
    long res1Long = a *b;
    long res2Long = (long)(a*b);
    long res3Long = (long)(a)*b;
	//long long c = (long long)(a * b);
	std::cout <<"res1Long = "<< res1Long  << "\t a*b = "<< a *b<< "\n";
	std::cout <<"res2Long = "<< res2Long  << "\t (long)( a*b) = "<< (long)( a*b)<< "\n";
	std::cout <<"res3Long = "<< res3Long  << "\t (long )( a)*b = "<< (long )( a)*b<< "\n";



    short s1 = 8192;
	short s2 = 64;
    short res1Short = s1*s2;
    short res2Short = (short)(s1*s2);
    short res3Short = (short)(s1)*s2;
    int  res4Int = s1*s2;
    int  res5Int = (short)(s1*s2);
    int  res6Int = (short)(s1)*s2;
	//long long c = (long long)(a * b);
	std::cout <<"res1Short = "<< res1Short  << "\t  s1*s2 = "<< s1*s2*63<< "\n";
	std::cout <<"res2Short = "<< res2Short  << "\t  (short)(s1*s2) = "<< (short)(s1*s2)<< "\n";
	std::cout <<"res3Short = "<< res3Short  << "\t  (short)(s1)*s2 = "<< (short)(s1)*s2<< "\n";
	std::cout <<"res4Int = "<< res4Int  << "\t  s1*s2 = "<< s1*s2<< "\n";
	std::cout <<"res5Int = "<< res5Int  << "\t  (short)(s1*s2) = "<< (short)(s1*s2)<< "\n";
	std::cout <<"res6Int = "<< res6Int  << "\t  (short)(s1)*s2 = "<< (short)(s1)*s2<< "\n";

	return 0;
}