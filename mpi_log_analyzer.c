#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

int main(int argc, char *argv[]) {
    int rank, size;
    FILE *file;
    int warning_count = 0, error_count = 0;
    int global_warning_count = 0, global_error_count = 0;
    char line[MAX_LINE_LENGTH];

    if (argc < 2) {
        if (rank == 0) {
            printf("Usage: %s <log_file_path>\n", argv[0]);
        }
        return 1;
    }

    MPI_Init(&argc, &argv); // Start MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get process ID
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get total number of processes

    double start_time = MPI_Wtime(); // Start timing

    // First, we count total lines (only by rank 0)
    int total_lines = 0;

    if (rank == 0) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        while (fgets(line, MAX_LINE_LENGTH, file)) {
            total_lines++;
        }
        fclose(file);
    }

    // Broadcast total lines to all processes
    MPI_Bcast(&total_lines, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate start and end line number for each process
    int lines_per_process = total_lines / size;
    int start_line = rank * lines_per_process;
    int end_line = (rank == size - 1) ? total_lines : start_line + lines_per_process;

    // Reopen file and read only assigned lines
    file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error reopening file");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int current_line = 0;
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (current_line >= start_line && current_line < end_line) {
            if (strstr(line, "WARNING")) warning_count++;
            if (strstr(line, "ERROR")) error_count++;
        }
        current_line++;
    }

    fclose(file);

    // Reduce all process results into rank 0
    MPI_Reduce(&warning_count, &global_warning_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&error_count, &global_error_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime(); // End timing

    // Rank 0 prints the result
    if (rank == 0) {
        printf("Number of processes used: %d\n", size);
        printf("Total WARNING lines: %d\n", global_warning_count);
        printf("Total ERROR lines: %d\n", global_error_count);
        printf("Execution time (MPI): %.4f seconds\n", end_time - start_time);
    }

    MPI_Finalize(); // Clean up
    return 0;
}
