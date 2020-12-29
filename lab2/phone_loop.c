#include <stdio.h>
#include <stdlib.h>

int main(){
   char phone[11];
   int num;
   int count = 0;

   scanf("%s", phone);
   while (scanf("%d", &num) != EOF){
	   if (num == -1){
	       printf("%s\n", phone);}
	   else if (num >= 0 && num <= 9){
	       printf("%c\n", phone[num]);}
	   else {
               count += 1;
	       printf("ERROR\n");}
}
   if (count == 0){ 
	return 0;}
   else {
	return 1;}
}
