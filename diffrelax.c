#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PARAMS 5
#define DISP_WIDTH 12
#define DISP_PRECN 8

double** malloc_array(int);

int main(int argc, char *argv[])
{
    /*
        Parameters:
        1. File containing space-separated square array of doubles (string)
        2. Data array dimension (int)
        3. Number of threads (int)
        4. Precision (double)

        Examples:
        $ ./diffrelax path/to/array.dat 6 16 1e-6
    */

    FILE *data_file;
    double **data_array, **avg_array;
    double precision;
    int data_dim, num_threads;
    int precise_num, avg_num;
    int i, j;

    if (argc != NUM_PARAMS) {
        printf("error: incorrect number of parameters, aborting ...\n");
        return 1;
    }

    /* Convert arguments to appropriate types */
    data_dim = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    precision = atof(argv[4]);

    printf("log: data_file=%s\n", argv[1]);
    printf("log: data_dim=%d\n", data_dim);
    printf("log: num_threads=%d\n", num_threads);
    printf("log: precision=%*.*f\n", DISP_WIDTH, DISP_PRECN, precision);
    printf("------------------------------------------------------------\n");

    /* Allocate memory for 2D array */
    data_array = malloc_array(data_dim);
    if (data_array == NULL) {
        printf("error: could not allocate memory for 2D array, aborting ...\n");
        return 1;
    }

    /* Open array file for reading */
    data_file = fopen(argv[1], "r");
    if (data_file == NULL) {
        printf("error: could not open data file, aborting ...\n");
        return 1;
    }

    /* Store data in 2D array */
    printf("log(data_array):\n");
    for (i = 0; i < data_dim; i++) {
        for (j = 0; j < data_dim; j++) {
            fscanf(data_file, "%lf", &data_array[i][j]);
            printf("%*.*f ", DISP_WIDTH, DISP_PRECN, data_array[i][j]);
        }
        fgetc(data_file);
        putchar('\n');
    }
    putchar('\n');

    fclose(data_file);

    for (;;) {
        /* Prepare 2D array for neighbour averages */
        avg_array = malloc_array(data_dim);
        if (avg_array == NULL) {
            printf("error: could not allocate memory for 2D array, aborting ...\n");
            return 1;
        }

        /* Average the four neighbours of non-boundary numbers */
        printf("log(avg_array):\n");
        for (i = 1; i < data_dim - 1; i++) {
            for (j = 1; j < data_dim - 1; j++) {
                avg_array[i][j] = (data_array[i - 1][j] + data_array[i][j - 1]
                        + data_array[i][j + 1] + data_array[i + 1][j]) / 4.0f;
                printf("%*.*f ", DISP_WIDTH, DISP_PRECN, avg_array[i][j]);
            }
            putchar('\n');
        }
        putchar('\n');

        /* Check if all values are within desired precision */
        precise_num = 0;
        printf("log(diff):\n");
        for (i = 1; i < data_dim - 1; i++) {
            for (j = 1; j < data_dim - 1; j++) {
                double diff = fabs(data_array[i][j] - avg_array[i][j]);
                printf("%*.*f ", DISP_WIDTH, DISP_PRECN, diff);
                if (diff < precision) precise_num++;
            }
            putchar('\n');
        }
        putchar('\n');

        avg_num = (data_dim - 2) * (data_dim - 2);
        printf("log: precise_num=%d/%d [diff < %*.*f]\n", precise_num, avg_num,
                DISP_WIDTH, DISP_PRECN, precision);
        printf("------------------------------------------------------------\n");

        /* Fill average results array with boundary numbers */
        printf("log(data_array):\n");
        for (i = 0; i < data_dim; i++) {
            for (j = 0; j < data_dim; j++) {
                if (i == 0 || i == data_dim - 1 || j == 0 || j == data_dim - 1) {
                    avg_array[i][j] = data_array[i][j];
                }
                printf("%*.*f ", DISP_WIDTH, DISP_PRECN, avg_array[i][j]);
            }
            putchar('\n');
        }
        putchar('\n');

        /* Deallocate memory for 2D array */
        for (i = 0; i < data_dim; i++) {
            free(data_array[i]);
        }
        free(data_array);

        if (precise_num == avg_num) {
            /* Deallocate memory for 2D array */
            for (i = 0; i < data_dim; i++) {
                free(avg_array[i]);
            }
            free(avg_array);
            return 0;
        }

        data_array = avg_array;
        avg_array = NULL;
    }
}

double** malloc_array(int data_dim) {
    int i;
    double **data_array = (double **) malloc(data_dim * sizeof(double *));
    if (data_array == NULL) return NULL;
    for (i = 0; i < data_dim; i++) {
        data_array[i] = (double *) malloc(data_dim * sizeof(double));
        if (data_array == NULL) return NULL;
    }
    return data_array;
}
