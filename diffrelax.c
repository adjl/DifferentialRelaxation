#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PARAMS 5
#define true 1
#define false 0

typedef int boolean;

int main(int argc, char *argv[])
{
    /*
        Parameters:
        1. Name of file containing space-separated square array of doubles (char *)
        2. Array dimension (int)
        3. Number of threads (int)
        4. Precision (double), 1e-6 preferred
        5. DEBUG (char *), option to enable debugging statements

        Examples:
        $ ./diffrelax values.txt 3 1 0.001
        $ ./diffrelax path/to/values.txt 6 4 1e-6 DEBUG
    */

    double **values;
    int dimension;
    int num_threads;
    double precision;

    /* Test and set debug mode */
    boolean debug = false;
    if (argc > NUM_PARAMS && strcmp(argv[NUM_PARAMS], "DEBUG") == 0) {
        debug = true;
    }

    if (debug) {
        printf("debug: data_file(char *)=%s\n", argv[1]);
        printf("debug: dimension(char *)=%s\n", argv[2]);
        printf("debug: num_threads(char *)=%s\n", argv[3]);
        printf("debug: precision(char *)=%s\n", argv[4]);
    }

    /* Convert arguments to appropriate types */
    dimension = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    precision = atof(argv[4]);

    if (debug) {
        printf("debug: dimension(int)=%d\n", dimension);
        printf("debug: num_threads(int)=%d\n", num_threads);
        printf("debug: precision(double)=%f\n", precision);
    }

    /* Allocate memory for 2D array */
    values = (double**) malloc(dimension * sizeof(double*));
    if (values == NULL) {
        fprintf(stdout, "error: could not allocate memory for 2D array, aborting ...\n");
        return 1;
    }

    /* Deallocate memory */
    free(values);

    return 0;
}
