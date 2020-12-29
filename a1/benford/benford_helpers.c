#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int count = 0;
	 while (num != 0){
	 	num = num / BASE;
		++count;
	 }
    return count;
}

int get_ith_from_right(int num, int i) {
	 for (int m = 0; m<i; m++) {
		num = num / BASE;
	}
    return num % BASE;
}

int get_ith_from_left(int num, int i) {
    int a, result;
	 a = count_digits(num) - i - 1;
	 result = get_ith_from_right(num, a);
	 
    return result;
}

void add_to_tally(int num, int i, int *tally) {
    int a;
	 int b;
	 
	 a = get_ith_from_left(num, i);
	 b = tally[a] + 1;
	 tally[a] = b;
	 return;
}
