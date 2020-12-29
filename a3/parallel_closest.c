#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    
    // if n < 4, or pdmax == 0, directly return the result
    if (n < 4 || pdmax == 0) {
        return closest_serial(p, n);
    } 

    int mid = n / 2;

    //pipe for the left and right children processer through.
    int fd[2][2];
    for (int i = 0; i <2; i++) {
        if (pipe(fd[i]) < 0) {
            perror("pipe");
            exit(1);
        }

        //call fork
        int result = fork();
        if (result < 0) {
            perror("fork");
            exit(1);
        } else if (result == 0) {
            double closest = closest_parallel(p + i * mid, mid + i * (n % 2), pdmax - 1, pcount);
            
            // child need to write to the pipe so close reading end            
            if (close(fd[i][0]) == -1) {            
                perror("close reading end from inside child");            
                exit(1);            
            }

            // write the distance to the pipe as a double           
            if (write(fd[i][1], &closest, sizeof(double)) != sizeof(double)) {                
                perror("write from child to pipe");                
                exit(1);            
            }

            // finish this pipe, close it            
            if (close(fd[i][1]) == -1) {                
                perror("close pipe after writing");                
                exit(1);            
            }

            exit(*pcount);
        
        } else {
            // close the end of the pipe that do not used.            
            if (close(fd[i][1]) == -1) {                
                perror("close writing end of pipe in parent");
                exit(1);
            }
        }
    }

    //wait for both child processes to complete and update the pcount
    for (int i = 0; i < 2; i++) {
	 	 int status;
        if (wait(&status) == -1) {
            perror("wait");
            exit(1);
        }
        if (WIFEXITED(status)) {
            *pcount += WEXITSTATUS(status);
        }
        
    }
    *pcount += 2; //for the parent pipe, it should add its children.

    //read from the two pipes to retrieve the results from the two child processes
    double distance[2];
    int contribution;
    for (int i = 0; i < 2; i++) {
        if (read(fd[i][0], &contribution, sizeof(double)) != sizeof(double)) {            
            perror("reading from pipe from a child");            
            exit(1);        
        }
        distance[i] = contribution;
    }
	 
    //close all pipe
	for (int i = 0; i < 2; i++) {
        if (close(fd[i][0]) == -1) {            
            perror("close");            
            exit(1);        
        }
        distance[i] = contribution;
    }


    // Find the smaller of two distances 
    double d = min(distance[0], distance[1]);

    // Build an array strip[] that contains points close (closer than d) to the line passing through the middle point.
    struct Point *strip = malloc(sizeof(struct Point) * n);
    if (strip == NULL) {
        perror("malloc");
        exit(1);
    }

    int j = 0;
    for (int i = 0; i < n; i++) {
        if (abs(p[i].x - p[mid].x) < d) {
            strip[j] = p[i], j++;
        }
    }

    // Find the closest points in strip.  Return the minimum of d and closest distance in strip[].
    double new_min = min(d, strip_closest(strip, j, d));
    free(strip);

    return new_min;
}

