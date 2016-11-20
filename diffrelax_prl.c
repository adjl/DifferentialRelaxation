#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PARAMS 5
#define DISP_WIDTH 12
#define DISP_PRECN 8

typedef struct {
    pthread_mutex_t *num_precise_mx;
    double **data_array, **avg_array;
    double precision;
    int *num_precise;
    int thread_id, avg_dim;
    int start_cell_i, stop_cell_i;
} pthread_args;

void init_pthread_args(pthread_args *, pthread_mutex_t *, double **, double **, double, int *, int);
void idle_thread(pthread_args *);
void calc_cell_avg(pthread_args *);
void fill_boundary_cells(double **, double **, int);
void load_values_to_array(FILE *, double **, int);
double** malloc_array(int);

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
    pthread_mutex_t num_precise_mx;
    pthread_t *threads;
    pthread_args *thread_args;
    double **data_array;
    double precision;
    int *thread_load;
    int data_dim, num_threads, num_avg;
    int fewer_threads_than_cells;
    int runs = 0;

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
    load_values_to_array(data_file, data_array, data_dim);
    fclose(data_file);

    num_avg = (data_dim - 2) * (data_dim - 2);
    fewer_threads_than_cells = num_threads < num_avg;
    if (fewer_threads_than_cells) {
        int extra_load, i;
        thread_load = (int *) malloc(num_threads * sizeof(int));
        if (thread_load == NULL) {
            printf("error: could not allocate memory for thread load array, aborting ...\n");
            return 1;
        }
        if (num_avg / 2 > num_avg - num_threads) {
            extra_load = num_avg - num_threads;
        } else {
            extra_load = num_avg % num_threads;
        }
        for (i = num_threads - 1; i >= 0; i--) {
            thread_load[i] += num_avg / num_threads;
            if (extra_load-- > 0) {
                thread_load[i]++;
            }
        }
    }

    pthread_mutex_init(&num_precise_mx, NULL);
    threads = (pthread_t *) malloc(num_threads * sizeof(pthread_t));
    if (threads == NULL) {
        printf("error: could not allocate memory for thread pointers, aborting... \n");
        return 1;
    }
    thread_args = (pthread_args *) malloc(num_threads * sizeof(pthread_args));
    if (thread_args == NULL) {
        printf("error: could not allocate memory for thread argument pointers, aborting ...\n");
        return 1;
    }

    for (;;) {
        double **avg_array;
        int num_precise, error, i;

        /* Allocate memory for 2D neighbour averages array */
        avg_array = malloc_array(data_dim);
        if (avg_array == NULL) {
            printf("error: could not allocate memory for 2D array, aborting ...\n");
            return 1;
        }

        /** Average the four neighbours of non-boundary numbers
            Calculate difference between between new and previous values
            Count number of results within desired precision */

        if (fewer_threads_than_cells) {
            int curr_cell_i;

            for (i = 0, curr_cell_i = 0; i < num_threads; curr_cell_i += thread_load[i], i++) {
                pthread_t thread;
                pthread_args args;

                init_pthread_args(&args, &num_precise_mx, data_array, avg_array, precision, &num_precise, data_dim);
                args.thread_id = i;
                args.start_cell_i = curr_cell_i;
                args.stop_cell_i = curr_cell_i + thread_load[i];
                thread_args[i] = args;
                error = pthread_create(&thread, NULL, (void *(*) (void *)) calc_cell_avg, (void *) &thread_args[i]);
                threads[i] = thread;
                if (error != 0) {
                    printf("error: could not create thread %d, aborting ...\n", i);
                    return 1;
                }
            }
        } else {
            for (i = 0; i < num_threads; i++) {
                pthread_t thread;
                pthread_args args;

                init_pthread_args(&args, &num_precise_mx, data_array, avg_array, precision, &num_precise, data_dim);
                args.thread_id = i;
                args.start_cell_i = i;
                args.stop_cell_i = i + 1;
                thread_args[i] = args;

                if (i < num_avg) {
                    error = pthread_create(&thread, NULL, (void *(*) (void *)) calc_cell_avg, (void *) &thread_args[i]);
                } else {
                    error = pthread_create(&thread, NULL, (void *(*) (void *)) idle_thread, (void *) &thread_args[i]);
                }

                threads[i] = thread;
                if (error != 0) {
                    printf("error: could not create thread %d, aborting ...\n", i);
                    return 1;
                }
            }
        }

        for (i = 0; i < num_threads; i++) {
            error = pthread_join(threads[i], NULL);
            if (error != 0) {
                printf("error: could not join on thread %d, aborting ...\n", i);
                return 1;
            }
        }

        printf("\nlog: runs=%d\n", ++runs);
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

            pthread_mutex_destroy(&num_precise_mx);
            free(threads);
            free(thread_args);
            free(thread_load);

            return 0;
        }

        /* Use filled averages array as next data array */
        data_array = avg_array;
        avg_array = NULL;
        num_precise = 0;
    }
}

void init_pthread_args(pthread_args *args, pthread_mutex_t *num_precise_mx, double **data_array, double **avg_array, double precision, int *num_precise, int data_dim)
{
    args->num_precise_mx = num_precise_mx;
    args->data_array = data_array;
    args->avg_array = avg_array;
    args->precision = precision;
    args->num_precise = num_precise;
    args->avg_dim = data_dim - 2;
}

void idle_thread(pthread_args *args)
{
    printf("thread %d being idle ...\n", args->thread_id);
}

void calc_cell_avg(pthread_args *args)
{
    double **data_array, **avg_array;
    double diff;
    int i, j, k;

    data_array = args->data_array;
    avg_array = args->avg_array;

    for (k = args->start_cell_i; k < args->stop_cell_i; k++) {
        i = k / args->avg_dim + 1;
        j = k % args->avg_dim + 1;

        avg_array[i][j] = (data_array[i - 1][j] + data_array[i][j - 1]
                + data_array[i][j + 1] + data_array[i + 1][j]) / 4.0f;
        diff = fabs(data_array[i][j] - avg_array[i][j]);

        printf("thread %d calculating avg_array[%d][%d]=%*.*f[%*.*f]\n",
                args->thread_id, i, j,
                DISP_WIDTH, DISP_PRECN, avg_array[i][j],
                DISP_WIDTH, DISP_PRECN, diff);

        if (diff < args->precision) {
            pthread_mutex_lock(args->num_precise_mx);
            (*args->num_precise)++;
            pthread_mutex_unlock(args->num_precise_mx);
            printf("\tthread %d: diff within precision, num_precise=%d\n",
                    args->thread_id, *args->num_precise);
        }
    }
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

void load_values_to_array(FILE *data_file, double **data_array, int data_dim)
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
