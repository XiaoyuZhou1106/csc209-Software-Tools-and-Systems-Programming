#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }
	 
	 else {
	 	int num_row;
		int num_col;
		int times;
		int item;
		
		num_row = strtol(argv[1], NULL, 10);
		num_col = strtol(argv[2], NULL, 10);
		times = strtol(argv[3], NULL, 10);
		
		int i = 0;
		int arr[num_col * num_row];
		while (scanf("%d", &item) == 1) {
			arr[i] = item;
			i++;
		}
		
		for (int j = 0; j < times; j++) {
			print_state(arr, num_row, num_col);
			update_state(arr, num_row, num_col);
		}
		return 0;
	 }

    
}
