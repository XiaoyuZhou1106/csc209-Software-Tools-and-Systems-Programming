#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }
	 else if (argc == 2) {
	 	int arr[BASE] = {0};
	 	int pos;
		int num;
		pos = strtol(argv[1], NULL, 10);
	 	while (scanf("%d", &num) != EOF) {
			add_to_tally(num, pos, arr);
		}
		for (int m = 0; m < BASE; m++) {
			printf("%ds: %d\n", m, arr[m]);
		}
		return 0;
	 }
	 else if (argc == 3) {
	 	int arr[BASE] = {0};
		int pos;
		pos = strtol(argv[1], NULL, 10);
		int integer;
		FILE *sample;
		
		sample = fopen(argv[2], "r");
		while (fscanf(sample, "%d", &integer) == 1) {
			add_to_tally(integer, pos, arr);
		}
		for (int m = 0; m < BASE; m++) {
			printf("%ds: %d\n", m, arr[m]);
		}
		fclose(sample);
		return 0;
	 }
}
