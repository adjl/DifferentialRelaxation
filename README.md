# CM30225 Parallel Computing: Shared Memory Programming

## To the reader
I have written this README in Markdown. If you are unfamiliar with it, or reading this file as plaintext, then please note that:
  # This indicates a heading
  ## This a subheading
  ### And subsubheading, and so on...


## Building
- To build both the parallel and sequential versions of the program, run `make` in the directory containing the Makefile.
- To build just the parallel or sequential version, run `make prl` and `make seq`, respectively.
- See the Makefile for details.


## Running
- To run either of the parallel and sequential version, run `./diffrelax_prl` or `./diffrelax_seq` with the following parameters in this particular order:
  1. filepath to a space-separated square array of doubles,
  2. array dimension,
  3. number of threads to use (ignored in the sequential version), and
  4. precision to work to (can be in decimal or exponential format, e.g., 0.0001, 1e-8).
- Example usage:
  ```bash
  $ ./diffrelax_prl data/array18.dat 18 16 1e-6
  $ ./differelax_seq data/array8.dat 8 0 1e-4 # Note: parm[3] is still required, just ignored for seq
  ```
- The program does not perform error-checking, so please be nice and give the correct arguments. In particular, ensure that:
  1. The array is formatted correctly in the file (see the sample array files).
  2. The passed array dimension matches that of the actual array.
  3. The precision is a sane value.


### Options
1. To get information on thread activity in the parallel version, set the VERBOSE flag in diffrelax_prl.c.
  - Enabling this will display which cells (indices) the threads are working on, their calculated averages and precisions, as well as highlight the (in)activity of idle threads.
  - For correctness testing and file comparison, unsetting VERBOSE is recommended, because the format matches that of the sequential version, making it suitable for use with `diff`.
    - A spurious error occurs when num_threads does not match in both files (as explained above, this value is ignored in the sequential version, having no effect on the output).
  - In addition, setting this mode will show thread exit messages, to show that they are killed cleanly.
2. You can adjust the display width and precision of the values in the output. To do so, open and change the settings of the DISP_WIDTH and DISP_PRECN constants.
  - These are available in both diffrelax_prl.c and diffrelax_seq.c


### Input arrays
- This project comes with several arrays ready to be used by the program, which are located in data/ and saved as .dat files. The number in the filename denotes the array's dimension, for convenience. A random array generator script is also included, which can be run and redirected to a file by `./gen_rand_array.py 9 > array9.dat`.
  - The sample arrays in data/ have dimensions 4-8, but there is a gargantuan 18x18 array in scalinvest/
  - Please refer to the sample files in creating custom array files to work with; the values on a row are separated by a single whitespace, with each row separated by a newline.


## Description of solution and approach
At a roughly (medium to) high level, my approach is as follows:
  1. Load the data array, the one containing the original values.
  2. Compute how to split (partition) work amongst threads; assign single or ranges of cells to each one.
  3. Average each cell using the thread assigned to it, storing the results in an 'averages' array.
  4. Check if the new value is within precision by calculating its difference from the previous value.
  5. If the new value is within precision, increment the counter of precise numbers.
  6. If the number of precise numbers matches that of the number of cells to average in the array, then we are done.
  7. Otherwise, fill in the boundary values of the current average array and treat is as the next data array (freeing the memory of the previous one).
  8. Repeat from (3), with the same assignment of threads to cells, until all numbers are within precision.


### Thread workload partitioning
Keywords: num_threads = no. of threads
          num_cells = no. of cells (elements) to compute the average of, [non-boundary values]
          extra_cells = num_cells - num_threads, those that cannot be "paired up" with a thread
a. If num_threads == num_cells, then every thread works on 1 cell (1:1).
b. If num_threads > num_cells, similar to (a) but the extra threads will have nothing to do.
c. If extra_cells <= half of num_cells, then some of the threads will double their workload to make up for the extra cells, while the rest of the threads (if any) continue to work on 1 cell.
d. If extra_cells > half of num_cells, then the extra cells will be evenly divided amongst the threads, following the pigeonhole principle, such that the biggest difference in workload between any two threads is at most 1 (e.g. workload of [2,3,3], NOT [2,2,4]).
Note: In (c) and (d), threads work on a _range_ of cells, not just a single one.


### Mutual exclusion and synchronisation
- A mutex is used to protect the counter of precise numbers (step 5), as it is shared and updated amongst threads.
- Because a thread cannot continue into the next iteration of average calculation without the other threads having finished their current pass, then they would all need to be in sync. Synchronisation is done using barriers, because the program (or threads?) flow is similar to that of supersteps.
  Delving into implementation details, the threads, once created, do not actually start their routine. This is because the main thread still has to prepare for the next round of calculations, e.g., creating a new averages array, updating the threads' references to swapped arrays. It is only when the main thread is done that the remaining threads can begin their calculations.
  Similarly, while the other threads are working, the main thread is blocked from printing off the entire array, and from deciding whether the job is done or due another pass.
  As mentioned above, these threads are synchronised using barriers, but because the main thread is involved, the barriers would have to unblock on num_threads + 1, to account for main.
- As an aside, I safely killed off the child threads (by unblocking them from a barrier and letting them exit their routine), because the effect of destroying a barrier with threads still blocked on it is undefined, according to POSIX.


## Correctness Testing
I performed my correctness testing by comparing the results of the parallel and sequential versions of the program for a given input array. For the parallel version, I also made multiple runs of it with varying numbers of threads, to check my work partitioning method.

The sequential result of the sample array with dimension n (data/array<n>.dat) is stored in results_seq/array<n>_seq.res, and its corresponding parallel results in results_prl/array<n>/array<n>-<t>_prl.res, where t = no. of threads used in that run.

Checking is then done via the following command (change this to suit your shell/environment):
```bash
for n in {4..8}; do
  for file in results_prl/array$n/array$n-*_prl.res; do
    diff -dy results_seq/array$n_seq.res $file | less
    # `diff -q` could be useful but it gives spurious warnings when num_threads differ, which is
    # ignored in the sequential version: diff could say the files differ, but they actually don't!
    # Better to check the array values themselves.
  done
done
```
A graphical file comparison tool like Meld also comes in handy.

Of course, you can also run and pipe the sequential and parallel results yourself:
```bash
$ make
$ ./differelax_seq data/array8.dat 8 0 1e-6 > ...
$ ./diffrelax_prl data/array8.dat 8 12 1e-6 > ...
```
Don't forget to set diffrelax_prl.c's VERBOSE constant to 0!


## Scalability investigation
### Sequentialisation
This particular program has issues regarding sequentialisation:
1. Reading the array from file: a given because the order of fscanf() calls matter, and there is no way to "index" the file---we can only read one value at a time.
2. Filling in boundary cells of the averages array, mainly an oversight on my part:
  a. If there are no free threads, then this is inherently sequential.
  b. In (a)'s case, I could have created extra threads just for this purpose, but, of course, it would have incurred overheads.
  c. I could have made use of extra (idle) threads, if there were any, but I didn't!
3. On a more general note, there are *some* sequential costs in using concurrency primitives:
  a. Barriers: _waiting_ for other tasks to finish IS a form of sequentialisation
  b. Mutex: in this program, only one thread can increment the counter at a time, which can be a hindrance given a sufficient number of threads. It might be better to split threads into smaller groups with each group having its own counter, all of which will be later summed together. But, of course, that would require more complexity to make it work correctly (e.g. each counter would have its own mutex, taking care that a thread updates the 'right' counter).


### Speedup && Efficiency
I investigated the program's behaviour when the size of the problem is significantly increased. I fed it an array of dimension 18 (16x16 => 256 cells), and timed how long it took to finish given varying numbers of threads, starting from num_threads=1 to 160.

The input array, (parallel and sequential) results, and, most importantly, the record of running times are kept in the scalinvest/ directory.

First off, sorry, I don't have pretty graphs. Judging from the times, it appears that all of the category of times increase as num_threads increases. This could be because the more threads there are, the greater the interference. This could also be in addition to my method of evenly splitting the workload (more communication and jumping across memory).

In terms of speedup and efficiency, it may be possible that I did not give the program a big enough array (problem). Also, as discussed above, there are various sequential parts in the program, which will ultimately limit how much the program can get from parallelism. And there is a slight issue with memory... (see below).


### Aside: Space / Memory
1. Given n threads, this program takes up O(n) memory for thread arguments, most of which are repeated (even pointers take up some space).
2. For a square array of dimension n, the space it takes up is O(n^2) memory.


## Miscellaneous
- This code was written following the ISO C90 style.
- The code is not optimised by the compiler (-O), and so could potentially run faster.
