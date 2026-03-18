
#include <stdio.h>
#include <limits.h>

typedef unsigned char  u8;
typedef unsigned int   u32;
typedef unsigned short u16;
typedef short i16;


// int pbuf->n
//nn equiv : int nByte = nData + 9 + 9 + FTS5_DATA_ZERO_PADDING;
// int nSpace  
/*struct Pbuff{
	int n;
	int nSpace;

}*/

#define fts5BufferGrow(nSpace, n, nn)                                          \
  ((u32)(n) + (u32)(nn) <= (u32)(nSpace)                       \
       ? 0                                                                     \
       : 1)//sqlite3Fts5BufferSize((pRc), (pBuf), (nn) + (pBuf)->n))

int main() {
	int n,nbyte,space;
	int nData=2147483637;
	n=10;
	nbyte=nData+26;
	space=800;
	//fts5BufferGrow(space,n,nbyte)==0
	if((u32)(n) + (u32) (nbyte) > 0 ){
		
		printf("true \n");
	}else{
		printf("else \n");

	}

	
	/*double c=(a+b)/8;
	printf("%lld\n",a);
	if(a<0){
		a=a-a;
		printf("bad execution %lld\n",a);
		b=a+2;	
	}else{
		a=2;
		a+=4;
		printf("not bad execution \n");
		
	}
	printf("c= \f",c);
	*/

return 0;
}
