#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_LINE_LENGTH 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <log_file_path>\n", argv[0]);
        return 1;
    }

    FILE *file;
    char **lines;
    char buffer[MAX_LINE_LENGTH];
    int line_count = 0;
    double start_time, end_time;

    // First pass: count total number of lines
    file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    while (fgets(buffer, MAX_LINE_LENGTH, file) != NULL) {
        line_count++;
    }
    fclose(file);

    // Allocate memory for storing all lines
    lines = (char **)malloc(line_count * sizeof(char *));
    file = fopen(argv[1], "r");

    int i = 0;
    while (fgets(buffer, MAX_LINE_LENGTH, file) != NULL && i < line_count) {
        lines[i] = strdup(buffer);  // Copy line into memory
        i++;
    }
    fclose(file);

    int warning_count = 0;
    int error_count = 0;

    start_time = omp_get_wtime();  // Start time

#pragma omp parallel
{
    int thread_count = omp_get_num_threads();
    #pragma omp single
    {
        printf("Using %d threads\n", thread_count);
    }

    #pragma omp for reduction(+:warning_count, error_count) //sum up each thread worning count and error count
    for (int i = 0; i < line_count; i++) {
        if (strstr(lines[i], "WARNING")) {
            warning_count++;
        }
        if (strstr(lines[i], "ERROR")) {
            error_count++;
        }
    }
}


    end_time = omp_get_wtime();  // End time

    printf("WARNING lines: %d\n", warning_count);
    printf("ERROR lines: %d\n", error_count);
    printf("Execution time (parallel OpenMP): %.4f seconds\n", end_time - start_time);

    // Free memory
    for (i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    return 0;
}
