#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PARAMS 5
#define DISP_WIDTH 12
#define DISP_PRECN 8

double** malloc_array(int);
void load_values_to_array(double **, int, FILE *);
void fill_boundary_cells(double **, double **, int);
int calc_avg_and_diff(double **, double **, int, double);

/** Parameters:
    1. File containing space-separated square array of doubles (string)
    2. Array dimension (int)
    3. Number of threads (int)
    4. Precision (double)

    Example: $ ./diffrelax path/to/array.dat 4 16 1e-6
    ---
    Test data:
    17.3198997974 1.8272498801 7.63466682327 51.9568205874
    2.75961348745 48.8679338506 11.9984764843 11.186690078
    49.0808084867 31.601551633 26.5627217303 0.665492935087
    33.7483243872 50.3427590377 33.6577443551 27.7679243712

    Test result:
    17.31989980   1.82724988   7.63466682  51.95682059
     2.75961349  12.62171406  12.87471945  11.18669008
    49.08080849  33.02527210  20.05580754   0.66549294
    33.74832439  50.34275904  33.65774436  27.76792437
*/
int main(int argc, char *argv[])
{
    FILE *data_file;
    double **data_array, **avg_array;
    double precision;
    int data_dim, num_threads;
    int num_precise, num_avg;
    int i;

    if (argc != NUM_PARAMS) {
        printf("error: incorrect number of parameters, aborting ...\n");
        return 1;
    }

    /* Convert arguments to appropriate types */
    /* TODO: Do error-checking */
    data_dim = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    precision = atof(argv[4]);

    printf("log: data_file=%s\n", argv[1]);
    printf("log: data_dim=%d\n", data_dim);
    printf("log: num_threads=%d\n", num_threads);
    printf("log: precision=%*.*f\n", DISP_WIDTH, DISP_PRECN, precision);
    printf("------------------------------------------------------------\n");

    /* Allocate memory for 2D data array */
    data_array = malloc_array(data_dim);
    if (data_array == NULL) {
        printf("error: could not allocate memory for 2D array, aborting ...\n");
        return 1;
    }

    /* Open data file */
    data_file = fopen(argv[1], "r");
    if (data_file == NULL) {
        printf("error: could not open data file, aborting ...\n");
        return 1;
    }

    /* Load data into array from file */
    load_values_to_array(data_array, data_dim, data_file);
    fclose(data_file);

    for (;;) {
        /* Allocate memory for 2D neighbour averages array */
        avg_array = malloc_array(data_dim);
        if (avg_array == NULL) {
            printf("error: could not allocate memory for 2D array, aborting ...\n");
            return 1;
        }

        /** Average the four neighbours of non-boundary numbers
            Calculate difference between between new and previous values
            Count number of results within desired precision */
        num_precise = calc_avg_and_diff(data_array, avg_array, data_dim, precision);
        num_avg = (data_dim - 2) * (data_dim - 2);

        printf("log: num_precise=%d/%d [diff < %*.*f]\n", num_precise, num_avg,
                DISP_WIDTH, DISP_PRECN, precision);
        printf("------------------------------------------------------------\n");

        /* Fill averages array with boundary numbers */
        fill_boundary_cells(data_array, avg_array, data_dim);

        /* Deallocate data array memory */
        for (i = 0; i < data_dim; i++) {
            free(data_array[i]);
        }
        free(data_array);

        /* All results within precision */
        if (num_precise == num_avg) {
            /* Deallocate averages array memory */
            for (i = 0; i < data_dim; i++) {
                free(avg_array[i]);
            }
            free(avg_array);

            return 0;
        }

        /* Use filled averages array as next data array */
        data_array = avg_array;
        avg_array = NULL;
    }
}

double** malloc_array(int data_dim)
{
    int i;
    double **data_array = (double **) malloc(data_dim * sizeof(double *));
    if (data_array == NULL) return NULL;
    for (i = 0; i < data_dim; i++) {
        data_array[i] = (double *) malloc(data_dim * sizeof(double));
        if (data_array == NULL) return NULL;
    }
    return data_array;
}

void load_values_to_array(double **data_array, int data_dim, FILE *data_file)
{
    int i, j;

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
}

void fill_boundary_cells(double **data_array, double **avg_array, int data_dim)
{
    int i, j;

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
}

int calc_avg_and_diff(double **data_array, double **avg_array, int data_dim, double precision)
{
    int num_precise = 0;
    int i, j;

    printf("log(avg_array[diff]):\n");
    for (i = 1; i < data_dim - 1; i++) {
        for (j = 1; j < data_dim - 1; j++) {
            double diff;
            avg_array[i][j] = (data_array[i - 1][j] + data_array[i][j - 1]
                    + data_array[i][j + 1] + data_array[i + 1][j]) / 4.0f;
            diff = fabs(data_array[i][j] - avg_array[i][j]);
            if (diff < precision) num_precise++;
            printf("%*.*f[%*.*f] ", DISP_WIDTH, DISP_PRECN, avg_array[i][j],
                    DISP_WIDTH, DISP_PRECN, diff);
        }
        putchar('\n');
    }
    putchar('\n');

    return num_precise;
}
