#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PARAMS 5
#define true 1
#define false 0

typedef int boolean;

void load_data_to_array(double **, int, FILE *);

int main(int argc, char *argv[])
{
    /*
        Parameters:
        1. Name of file containing space-separated square array of doubles (char *)
        2. Data array dimension (int)
        3. Number of threads (int)
        4. Precision (double), 1e-6 preferred
        5. DEBUG (char *), option to enable debugging statements

        Examples:
        $ ./diffrelax path/to/array.dat 6 16 1e-6 DEBUG
    */

    double **data_array;
    int data_dim;
    int num_threads;
    double precision;
    FILE *data_file;
    int i, j;
    double **avg_array;
    int num_precise;

    /* Test and set debug mode */
    boolean debug = false;
    if (argc > NUM_PARAMS && strcmp(argv[NUM_PARAMS], "DEBUG") == 0) {
        debug = true;
    }

    if (debug) {
        printf("debug: data_file(char *)=%s\n", argv[1]);
        printf("debug: data_dim(char *)=%s\n", argv[2]);
        printf("debug: num_threads(char *)=%s\n", argv[3]);
        printf("debug: precision(char *)=%s\n", argv[4]);
    }

    /* Convert arguments to appropriate types */
    data_dim = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    precision = atof(argv[4]);

    if (debug) {
        printf("debug: data_dim(int)=%d\n", data_dim);
        printf("debug: num_threads(int)=%d\n", num_threads);
        printf("debug: precision(double)=%.8f\n", precision);
    }

    /* Allocate memory for 2D array */
    data_array = (double **) malloc(data_dim * sizeof(double *));
    if (data_array == NULL) {
        printf("error: could not allocate memory for 2D array, aborting ...\n");
        return 1;
    }
    for (i = 0; i < data_dim; i++) {
        data_array[i] = (double *) malloc(data_dim * sizeof(double));
        if (data_array[i] == NULL) {
            printf("error: could not allocate memory for row %d of 2D array, aborting ...\n", i);
            return 1;
        }
    }

    /* Open array file for reading */
    data_file = fopen(argv[1], "r");
    if (data_file == NULL) {
        printf("error: could not open data file, aborting ...\n");
        return 1;
    }

    /* Store data in 2D array */
    load_data_to_array(data_array, data_dim, data_file);

    if (debug) {
        printf("debug(load_data_to_array):\n");
        for (i = 0; i < data_dim; i++) {
            for (j = 0; j < data_dim; j++) {
                printf("%.8f ", data_array[i][j]);
            }
            putchar('\n');
        }
    }

    /* Close file */
    fclose(data_file);

    /* Prepare 2D array for neighbour averages */
    avg_array = (double **) malloc(data_dim * sizeof(double *));
    for (i = 0; i < data_dim; i++) {
        avg_array[i] = (double *) malloc(data_dim * sizeof(double));
    }

    /* Average the four neighbours of non-boundary numbers */
    if (debug) printf("debug(avg_array):\n");
    for (i = 1; i < data_dim - 1; i++) {
        for (j = 1; j < data_dim - 1; j++) {
            avg_array[i][j] = (data_array[i - 1][j] + data_array[i][j - 1]
                    + data_array[i][j + 1] + data_array[i + 1][j]) / 4.0f;
            if (debug) printf("%.8f ", avg_array[i][j]);
        }
        if (debug) putchar('\n');
    }

    /* Check if all values are within desired precision */
    num_precise = 0;
    if (debug) printf("debug(diff):\n");
    for (i = 1; i < data_dim - 1; i++) {
        for (j = 1; j < data_dim - 1; j++) {
            double diff = fabs(data_array[i][j] - avg_array[i][j]);
            if (debug) printf("%.8f ", diff);
            if (diff < precision) {
                num_precise++;
            }
        }
        if (debug) putchar('\n');
    }
    if (debug) printf("debug(num_precise): %d\n", num_precise);

    /* Deallocate memory for 2D array */
    for (i = 0; i < data_dim; i++) {
        free(avg_array[i]);
    }
    free(avg_array);
    for (i = 0; i < data_dim; i++) {
        free(data_array[i]);
    }
    free(data_array);

    return 0;
}

void load_data_to_array(double **data_array, int data_dim, FILE *data_file)
{
    int i, j;
    for (i = 0; i < data_dim; i++) {
        for (j = 0; j < data_dim; j++) {
            fscanf(data_file, "%lf", &data_array[i][j]);
        }
        fgetc(data_file);
    }
}
