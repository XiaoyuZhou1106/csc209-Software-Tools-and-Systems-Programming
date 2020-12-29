#include <stdio.h>
#include <stdlib.h>


void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
	int arr[num_rows*num_cols];
	 for (int i = 0; i < (num_rows -1)* num_cols; i++) {
	 	if (i > num_cols && ((i +1) % num_cols != 0) && (i % num_cols != 0)) {
			int count = 0;
			if (board[i - num_cols -1] == 1) {
				++count;
			}
			if (board[i - num_cols] == 1) {
				++count;
			}
			if (board[i - num_cols +1] == 1) {
				++count;
			}
			if (board[i -1] == 1) {
				++count;
			}
			if (board[i + 1] == 1) {
				++count;
			}
			if (board[i + num_cols -1] == 1) {
				++count;
			}
			if (board[i + num_cols] == 1) {
				++count;
			}
			if (board[i + num_cols +1] == 1) {
				++count;
			}
			if (board[i] == 1 && (count < 2 || count >3)) {
				arr[i] = 0;
			}else if (board[i] == 0 && (count == 2 || count == 3)) {
				arr[i] = 1;
			}else {arr[i]=board[i];}
		}
	 }
	 
	 for (int i = 0; i < (num_rows -1)* num_cols; i++) {
	 	if (i > num_cols && ((i +1) % num_cols != 0) && (i % num_cols != 0)) {
			board[i] = arr[i];
		}
	 }
    return;
}
