#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PARAMS 5
#define DISP_WIDTH 12 /* Modify this to change the output width of the values */
#define DISP_PRECN 8  /* Likewise for the display precision */
#define VERBOSE 1     /* Toggle to 0 for piping output and to use with `diff` */

typedef struct {
    pthread_barrier_t *resume_barrier, *pause_barrier;
    pthread_mutex_t *num_precise_mx;
    double **data_array, **avg_array;
    double precision;
    int *all_precise, *num_precise;
    int avg_dim, id, load;
    int range_begin, range_end;
} pthread_args;

void just_idle(pthread_args *);
void calculate_cell_avg(pthread_args *);

void complete_and_print_array(double **, double **, int);
void load_data_values(FILE *, double **, int);
double** malloc_array(int);

/** Parameters:
    1. File containing space-separated square array of doubles (path)
    2. Array dimension (int)
    3. Number of threads (int)
    4. Precision (double)
    Example: $ ./diffrelax_prl data/array4.dat 4 4 1e-6 */
int main(int argc, char *argv[])
{
    FILE *data_file;
    pthread_barrier_t resume_barrier, pause_barrier;
    pthread_mutex_t num_precise_mx;
    pthread_args *thread_args;
    double **data_array;
    double precision;
    int data_dim, num_threads, num_cells;
    int all_precise, num_precise, run, err, i;

    /* Initialise variables ------------------------------------------------- */

    all_precise = 0;
    num_precise = 0;
    run = 0;

    /* Read arguments ------------------------------------------------------- */

    if (argc < NUM_PARAMS) {
        printf("error: too few arguments, aborting ...\n");
        return 1;
    }

    data_dim = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    precision = atof(argv[4]);

    printf("data_file=%s\n", argv[1]);
    printf("data_dim=%d\n", data_dim);
    printf("num_threads=%d\n", num_threads);
    printf("precision=%*.*f\n\n", DISP_WIDTH, DISP_PRECN, precision);

    /* Load values into data array ------------------------------------------ */

    data_array = malloc_array(data_dim);
    if (data_array == NULL) {
        printf("error: could not allocate memory for data array, aborting ...\n");
        return 1;
    }
    data_file = fopen(argv[1], "r");
    if (data_file == NULL) {
        printf("error: could not open data file, aborting ...\n");
        return 1;
    }
    load_data_values(data_file, data_array, data_dim);
    fclose(data_file);

    /* Initialise barriers and mutex ---------------------------------------- */

    err = pthread_barrier_init(&resume_barrier, NULL, num_threads + 1);
    if (err != 0) {
        printf("error: could not initialise barrier, aborting ...\n");
        return err;
    }
    err = pthread_barrier_init(&pause_barrier, NULL, num_threads + 1);
    if (err != 0) {
        printf("error: could not initialise barrier, aborting ...\n");
        return err;
    }
    err = pthread_mutex_init(&num_precise_mx, NULL);
    if (err != 0) {
        printf("error: could not initialise mutex, aborting ...\n");
        return err;
    }

    /* Initialise thread arguments ------------------------------------------ */

    thread_args = (pthread_args *) malloc(num_threads * sizeof(pthread_args));
    if (thread_args == NULL) {
        printf("error: could not allocate memory for thread arguments array, aborting ...\n");
        return 1;
    }
    for (i = 0; i < num_threads; i++) {
        pthread_args args;
        args.resume_barrier = &resume_barrier;
        args.pause_barrier = &pause_barrier;
        args.num_precise_mx = &num_precise_mx;
        args.precision = precision;
        args.all_precise = &all_precise;
        args.num_precise = &num_precise;
        args.avg_dim = data_dim - 2;
        args.id = i;
        args.load = 0;
        thread_args[i] = args;
    }

    /* Split work amongst threads ------------------------------------------- */

    num_cells = (data_dim - 2) * (data_dim - 2); /* Number of cells to average */
    if (num_threads < num_cells) {
        int extra_load;
        if (num_cells / 2 > num_cells - num_threads) {  /* Extra cells <= half of total */
            extra_load = num_cells - num_threads;
        } else {
            extra_load = num_cells % num_threads;
        }
        for (i = num_threads - 1; i >= 0; i--) {
            thread_args[i].load += num_cells / num_threads;
            if (extra_load-- > 0) thread_args[i].load++; /* Spread extra load AMAP */
        }
    }

    /* Create and initialise threads ---------------------------------------- */

    if (num_threads < num_cells) {
        int range_i;
        for (i = 0, range_i = 0; i < num_threads; range_i += thread_args[i].load, i++) {
            pthread_t thread;
            thread_args[i].range_begin = range_i; /* Work on range of cells */
            thread_args[i].range_end = range_i + thread_args[i].load;
            err = pthread_create(&thread, NULL,
                    (void * (*) (void *)) calculate_cell_avg,
                    (void *) &thread_args[i]);
            if (err != 0) {
                printf("error: could not create thread %d, aborting ...\n", i);
                return err;
            }
        }
    } else {
        for (i = 0; i < num_threads; i++) {
            pthread_t thread;
            thread_args[i].range_begin = i; /* Work on one cell */
            thread_args[i].range_end = i + 1;
            if (i < num_cells) {
                err = pthread_create(&thread, NULL,
                        (void * (*) (void *)) calculate_cell_avg,
                        (void *) &thread_args[i]);
            } else {
                err = pthread_create(&thread, NULL, /* Extra (idle) thread */
                        (void * (*) (void *)) just_idle,
                        (void *) &thread_args[i]);
            }
            if (err != 0) {
                printf("error: could not create thread %d, aborting ...\n", i);
                return err;
            }
        }
    }

    /* Main thread loop ----------------------------------------------------- */

    for (;;) {
        double **avg_array = malloc_array(data_dim);
        if (avg_array == NULL) {
            printf("error: could not allocate memory for averages array, aborting ...\n");
            return 1;
        }

        for (i = 0; i < num_threads; i++) { /* Update threads' array references */
            thread_args[i].data_array = data_array;
            thread_args[i].avg_array = avg_array;
        }

        pthread_barrier_wait(&resume_barrier);
        pthread_barrier_wait(&pause_barrier);

        printf("\nrun=%d num_precise=%d/%d [diff < %*.*f]\n", ++run,
                num_precise, num_cells, DISP_WIDTH, DISP_PRECN, precision);
        printf("-----------------------------------------------------------\n");

        complete_and_print_array(data_array, avg_array, data_dim);
        for (i = 0; i < data_dim; i++) { /* Discard old values */
            free(data_array[i]);
        }
        free(data_array);

        if (num_precise == num_cells) { /* All cells now within precision */
            all_precise = 1;
            pthread_barrier_wait(&resume_barrier);
            for (i = 0; i < data_dim; i++) {
                free(avg_array[i]);
            }
            free(avg_array);
            break;
        }

        data_array = avg_array; /* Use averages array as next data array */
        num_precise = 0;
    }

    /* Clean up after ourselves --------------------------------------------- */

    err = pthread_barrier_destroy(&resume_barrier);
    if (err != 0) {
        printf("error: could not destroy barrier, aborting ...\n");
        return err;
    }
    err = pthread_barrier_destroy(&pause_barrier);
    if (err != 0) {
        printf("error: could not destroy barrier, aborting ...\n");
        return err;
    }
    err = pthread_mutex_destroy(&num_precise_mx);
    if (err != 0) {
        printf("error: could not destroy mutex, aborting ...\n");
        return err;
    }
    free(thread_args);

    return 0;
}

void just_idle(pthread_args *args)
{
    for (;;) {
        pthread_barrier_wait(args->resume_barrier);
        if (*args->all_precise) {
            if (VERBOSE) printf("thread %d: had nothing to do ... :(\n", args->id);
            break;
        }
        if (VERBOSE) printf("thread %d: idling and feeling useless ...\n", args->id);
        pthread_barrier_wait(args->pause_barrier);
    }
}

void calculate_cell_avg(pthread_args *args)
{
    for (;;) {
        double **data_array, **avg_array;
        int k;

        pthread_barrier_wait(args->resume_barrier);
        if (*args->all_precise) {
            if (VERBOSE) printf("thread %d: job done, exiting ...\n", args->id);
            break;
        }

        data_array = args->data_array;
        avg_array = args->avg_array;

        for (k = args->range_begin; k < args->range_end; k++) {
            double diff;
            int i, j;

            i = k / args->avg_dim + 1;
            j = k % args->avg_dim + 1;

            /** Calculate the average of the cell's four neighbours
                Get the difference between the new and previous values
                Increment counter if it is within the given precision */

            avg_array[i][j] = (data_array[i - 1][j] + data_array[i][j - 1]
                    + data_array[i][j + 1] + data_array[i + 1][j]) / 4.0f;
            diff = fabs(data_array[i][j] - avg_array[i][j]);

            if (VERBOSE) {
                printf("thread %d: calculating avg_array[%d][%d]=%*.*f[%*.*f]\n",
                        args->id, i, j,
                        DISP_WIDTH, DISP_PRECN, avg_array[i][j],
                        DISP_WIDTH, DISP_PRECN, diff);
            }

            if (diff < args->precision) {
                int err = pthread_mutex_lock(args->num_precise_mx);
                if (err != 0) {
                    printf("error: could not lock mutex, aborting ...\n");
                    exit(EXIT_FAILURE);
                }
                (*args->num_precise)++;
                err = pthread_mutex_unlock(args->num_precise_mx);
                if (err != 0) {
                    printf("error: could not unlock mutex, aborting ...\n");
                    exit(EXIT_FAILURE);
                }
                if (VERBOSE) {
                    printf("\tthread %d: diff within precision, num_precise=%d\n",
                            args->id, *args->num_precise);
                }
            }
        }

        pthread_barrier_wait(args->pause_barrier);
    }
}

void complete_and_print_array(double **data_array, double **avg_array, int data_dim)
{
    int i, j;
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

void load_data_values(FILE *data_file, double **data_array, int data_dim)
{
    int i, j;
    for (i = 0; i < data_dim; i++) {
        for (j = 0; j < data_dim; j++) {
            fscanf(data_file, "%lf", &data_array[i][j]);
        }
        fgetc(data_file);
    }
}

double** malloc_array(int dim)
{
    int i;
    double **array = (double **) malloc(dim * sizeof(double *));
    if (array == NULL) return NULL;
    for (i = 0; i < dim; i++) {
        array[i] = (double *) malloc(dim * sizeof(double));
        if (array[i] == NULL) return NULL;
    }
    return array;
}
