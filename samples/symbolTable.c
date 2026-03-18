#include<stdio.h>

int main() {
    int y;     
    int x = 2; 
    printf("Enter one integer: "); 
    scanf("%d ", &y); 
    int result = x * y; 
    printf("le double de %d est: %d\n", y, result); 
    return 0;
}