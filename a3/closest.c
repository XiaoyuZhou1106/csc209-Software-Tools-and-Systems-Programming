#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "point.h"
#include "utilities_closest.h"
#include "serial_closest.h"
#include "parallel_closest.h"


void print_usage() {
    fprintf(stderr, "Usage: closest -f filename -d pdepth\n\n");
    fprintf(stderr, "    -d Maximum process tree depth\n");
    fprintf(stderr, "    -f File that contains the input points\n");

    exit(1);
}

int main(int argc, char **argv) {
    int n = -1;
    long pdepth = -1;
    char *filename = NULL;
    int pcount = 0;
    int c;

    // TODO: Parse the command line arguments
    // if the input is larger or smaller than 3, then, print the error and exit the program
    if (argc != 5) {
        print_usage();
    }

    // use getopt to get the content of command input.
    while ((c = getopt(argc, argv, "f:d:")) != -1) {
        switch (c) {
            case 'f':
                filename = optarg;
                break;
            case 'd':
                pdepth = atol(optarg);
                break;
            default:
                print_usage();
        }
    }

    // after getopt, if the filename or pdepth does not change, then print error and exit.
    if (filename == NULL || pdepth == -1) {
        print_usage();
    }


    // Read the points
    n = total_points(filename);
    struct Point points_arr[n];
    read_points(filename, points_arr);

    // Sort the points
    qsort(points_arr, n, sizeof(struct Point), compare_x);

    // Calculate the result using the parallel algorithm.
    double result_p = closest_parallel(points_arr, n, pdepth, &pcount);
    printf("The smallest distance: is %.2f (total worker processes: %d)\n", result_p, pcount);

    exit(0);
}
