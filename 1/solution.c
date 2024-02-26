#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */


 void merge(int arr[], int left, int mid, int right);
 void merge_sort(int arr[], int left, int right);



 struct my_context {
     char* name;
     FILE* file;

 };

 static struct my_context* my_context_new(const char* name) {
     struct my_context* ctx = malloc(sizeof(*ctx));
     ctx->name = strdup(name);
     ctx->file = NULL;
     return ctx;
 }

 static void my_context_delete(struct my_context* ctx) {
     free(ctx->name);
     free(ctx);
 }

 static void merge_files(const char* output_filename, const char** input_filenames, int num_files) {
     FILE* output_file = fopen(output_filename, "w");
     if (output_file == NULL) {
         printf("Error opening output file: %s\n", output_filename);
         return;
     }
     int* array = malloc(num_files * sizeof(int));
     int full_size = 0;
     for (int i = 0; i < num_files; i++) {
         FILE* input_file = fopen(input_filenames[i], "r");
         if (input_file == NULL) {
             printf("Error opening input file: %s\n", input_filenames[i]);
             continue;
         }

         int size = 0;
         int num;
         while (fscanf(input_file, "%d", &num) == 1) {
             size++;
         }

         fseek(input_file, 0, SEEK_SET);

         array = realloc(array, (full_size + size) * sizeof(int));
         for (int j = full_size; j < full_size + size; j++) {
             fscanf(input_file, "%d", &array[j]);
         }
         full_size = full_size + size;
         fclose(input_file);
     }
     merge_sort(array, 0, full_size - 1);

     for (int i = 0; i < full_size; i++) {
         fprintf(output_file, "%d ", array[i]);
     }

     fclose(output_file);


     free(array);
 }

 static int coroutine_func_f(void* context) {
     struct timespec curr_time, start_time;
     double accum, latency = get_latency();
     struct my_context* ctx = context;
     char* name = ctx->name;
     struct coro *this = coro_this();


     if( clock_gettime( CLOCK_MONOTONIC, &start_time) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }
     set_start_time(start_time);
     // Sort the file using merge sort
     int* arr = NULL;
     int size = 0;

     ctx->file = fopen(name, "r");
     if (ctx->file == NULL) {
         printf("Error opening file: %s\n", name);
         my_context_delete(ctx);
         return -1;
     }
     int num;
     while (fscanf(ctx->file, "%d", &num) == 1) {
         size++;

         if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
         {
           perror( "clock gettime" );
           return EXIT_FAILURE;
         }

         accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;

         if(accum >= latency){
           coro_yield();
           if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
           {
             perror( "clock gettime" );
             return EXIT_FAILURE;
           }
           double full_time = get_full_time(NULL);
           full_time += accum;
           set_full_time(full_time);
           set_start_time(curr_time);
           start_time = get_start_time();
         }

     }
     fseek(ctx->file, 0, SEEK_SET);
     arr = malloc(size * sizeof(int));
     for (int i = 0; i < size; i++) {
         fscanf(ctx->file, "%d", &arr[i]);

         if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
         {
           perror( "clock gettime" );
           return EXIT_FAILURE;
         }

         accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;

         if(accum >= latency){
           coro_yield();
           if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
           {
             perror( "clock gettime" );
             return EXIT_FAILURE;
           }
           double full_time = get_full_time(NULL);
           full_time += accum;
           set_full_time(full_time);
           set_start_time(curr_time);
           start_time = get_start_time();
         }

     }

     fclose(ctx->file);

     if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }

     accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;

     if(accum >= latency){
       coro_yield();
       if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
       {
         perror( "clock gettime" );
         return EXIT_FAILURE;
       }
       double full_time = get_full_time(NULL);
       full_time += accum;
       set_full_time(full_time);
       set_start_time(curr_time);
       start_time = get_start_time();
     }

     merge_sort(arr, 0, size - 1);  // Call the merge_sort function

     if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }

     accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;
     if(accum >= latency){
       coro_yield();
       if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
       {
         perror( "clock gettime" );
         return EXIT_FAILURE;
       }
       double full_time = get_full_time(NULL);
       full_time += accum;
       set_full_time(full_time);
       set_start_time(curr_time);
       start_time = get_start_time();
     }

     ctx->file = fopen(name, "w");
     if (ctx->file == NULL) {
         printf("Error opening file: %s\n", name);
         free(arr);
         my_context_delete(ctx);
         return -1;
     }

     for (int i = 0; i < size; i++) {
         fprintf(ctx->file, "%d ", arr[i]);

         if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
         {
           perror( "clock gettime" );
           return EXIT_FAILURE;
         }

         accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;

         if(accum >= latency){
           coro_yield();
           if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
           {
             perror( "clock gettime" );
             return EXIT_FAILURE;
           }
           double full_time = get_full_time(NULL);
           full_time += accum;
           set_full_time(full_time);
           set_start_time(curr_time);
           start_time = get_start_time();
         }

     }

     fclose(ctx->file);
     free(arr);

     if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }

     accum =(double)( curr_time.tv_nsec - start_time.tv_nsec ) / (double)1000000;
     if(accum >= latency){
       coro_yield();
       if( clock_gettime( CLOCK_MONOTONIC, &curr_time) == -1 )
       {
         perror( "clock gettime" );
         return EXIT_FAILURE;
       }
       double full_time = get_full_time(NULL);
       full_time += accum;
       set_full_time(full_time);
       set_start_time(curr_time);
       start_time = get_start_time();
     }
     my_context_delete(ctx);

     return 0;
 }

 void merge(int arr[], int l, int m, int r)
 {
     int i, j, k;
     int n1 = m - l + 1;
     int n2 = r - m;

     // Create temp arrays
     int L[n1], R[n2];

     // Copy data to temp arrays
     // L[] and R[]
     for (i = 0; i < n1; i++)
         L[i] = arr[l + i];
     for (j = 0; j < n2; j++)
         R[j] = arr[m + 1 + j];

     // Merge the temp arrays back
     // into arr[l..r]
     // Initial index of first subarray
     i = 0;

     // Initial index of second subarray
     j = 0;

     // Initial index of merged subarray
     k = l;
     while (i < n1 && j < n2) {


         if (L[i] <= R[j]) {
             arr[k] = L[i];
             i++;
         }
         else {
             arr[k] = R[j];
             j++;
         }
         k++;
     }

     // Copy the remaining elements
     // of L[], if there are any
     while (i < n1) {

         arr[k] = L[i];
         i++;
         k++;
     }

     // Copy the remaining elements of
     // R[], if there are any
     while (j < n2) {

         arr[k] = R[j];
         j++;
         k++;
     }
 }

 // l is for left index and r is
 // right index of the sub-array
 // of arr to be sorted
 void merge_sort(int arr[], int l, int r)
 {

     if (l < r) {
         // Same as (l+r)/2, but avoids
         // overflow for large l and h
         int m = l + (r - l) / 2;

         // Sort first and second halves
         merge_sort(arr, l, m);

         merge_sort(arr, m + 1, r);

         merge(arr, l, m, r);
     }
 }


 int main(int argc, char** argv) {

     if (argc < 4) {
         printf("Usage: %s <output_file> <input_file1> <input_file2> ...\n", argv[0]);
         return -1;
     }
     struct timespec stop, start;
     if( clock_gettime( CLOCK_MONOTONIC, &start) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }
     const char* output_filename = "output.txt";
     const int num_coro = atoi(argv[1]);
     double latency = atoi(argv[2]), full_work_time;
     const int num_files = argc - 3;
     int unprocessed;
     struct coro* c;
     /* Initialize our coroutine global cooperative scheduler. */
     coro_sched_init();
     //printf("HA\n" );
     //coro_set_quantum(latency);

     /* Start coroutines for sorting individual files. */

     for (int i = 0; i < num_coro; ++i) {
         coro_new(coroutine_func_f, my_context_new(argv[i + 3]), latency, false, c, i);
     }
     unprocessed = num_files - num_coro;

     while(unprocessed > 0){

       while ((c = coro_sched_wait()) != NULL) {
           //printf("fff%d\n", argc - unprocessed);
           //unprocessed--;
           set_func_arg(my_context_new(argv[argc - unprocessed]), c);
           //printf("%d\n", unprocessed);
           unprocessed--;
           if(unprocessed <=0){
             break;
           }
       }
     }


     while ((c = coro_sched_wait()) != NULL) {
         printf("coro with id %d run with time %f and the count switch was %lld\n", get_id(c), get_full_time(c), coro_switch_count(c));
         coro_delete(c);
     }

     /* Merge the sorted files into a new file. */
     merge_files(output_filename, (const char**)(argv + 3), num_files);

     if( clock_gettime( CLOCK_MONOTONIC, &stop) == -1 )
     {
       perror( "clock gettime" );
       return EXIT_FAILURE;
     }
     if (stop.tv_nsec >= start.tv_nsec){
         full_work_time = (double)( stop.tv_nsec - start.tv_nsec ) / (double)1000000;
     }
     else {
       full_work_time = (double)( start.tv_nsec - stop.tv_nsec ) / (double)1000000;
     }
     printf("Full Work Time %f\n", full_work_time);

     return 0;
 }
