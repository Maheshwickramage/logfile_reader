#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);  // Initialize MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get rank (process ID)
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Get number of processes

    if (argc < 2) {
        if (rank == 0) {
            printf("Usage: %s <log_file_path>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    double start_time = MPI_Wtime();

    // ---------- Step 1: Read file (only by rank 0) ----------
    FILE *file;
    int total_lines = 0;
    char **all_lines = NULL;
    char buffer[MAX_LINE_LENGTH];

    if (rank == 0) {
        file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Count lines first
        while (fgets(buffer, MAX_LINE_LENGTH, file)) {
            total_lines++;
        }

        rewind(file);

        // Allocate memory for all lines
        all_lines = (char **)malloc(total_lines * sizeof(char *));
        for (int i = 0; i < total_lines; i++) {
            fgets(buffer, MAX_LINE_LENGTH, file);
            all_lines[i] = strdup(buffer);
        }

        fclose(file);
    }

    // Broadcast total line count to all
    MPI_Bcast(&total_lines, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate lines per process
    int lines_per_proc = total_lines / size;
    int remainder = total_lines % size;
    int start = rank * lines_per_proc + (rank < remainder ? rank : remainder);
    int count = lines_per_proc + (rank < remainder ? 1 : 0);

    // Send chunks of lines to each process
    char **local_lines = (char **)malloc(count * sizeof(char *));
    if (rank == 0) {
        for (int p = 1; p < size; p++) {
            int p_start = p * lines_per_proc + (p < remainder ? p : remainder);
            int p_count = lines_per_proc + (p < remainder ? 1 : 0);
            for (int i = 0; i < p_count; i++) {
                int len = strlen(all_lines[p_start + i]) + 1;
                MPI_Send(&len, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                MPI_Send(all_lines[p_start + i], len, MPI_CHAR, p, 0, MPI_COMM_WORLD);
            }
        }

        for (int i = 0; i < count; i++) {
            local_lines[i] = all_lines[start + i];
        }

    } else {
        for (int i = 0; i < count; i++) {
            int len;
            MPI_Recv(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local_lines[i] = (char *)malloc(len * sizeof(char));
            MPI_Recv(local_lines[i], len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    // ---------- Step 2: OpenMP Parallel Counting ----------
    int local_warning = 0, local_error = 0;

    #pragma omp parallel for reduction(+:local_warning, local_error)
    for (int i = 0; i < count; i++) {
        if (strstr(local_lines[i], "WARNING")) local_warning++;
        if (strstr(local_lines[i], "ERROR")) local_error++;
    }

    // ---------- Step 3: MPI Reduce ----------
    int global_warning = 0, global_error = 0;

    MPI_Reduce(&local_warning, &global_warning, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_error, &global_error, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();

    // ---------- Step 4: Rank 0 Prints Results ----------
    if (rank == 0) {
        int threads = omp_get_max_threads();
        printf("Hybrid MPI + OpenMP Log Analyzer\n");
        printf("Processes: %d\n", size);
        printf("Threads per process: %d\n", threads);
        printf("Total WARNING lines: %d\n", global_warning);
        printf("Total ERROR lines: %d\n", global_error);
        printf("Execution time: %.4f seconds\n", end_time - start_time);
    }

    // ---------- Step 5: Cleanup ----------
    for (int i = 0; i < count; i++) {
        free(local_lines[i]);
    }
    free(local_lines);
    if (rank == 0) free(all_lines);

    MPI_Finalize();
    return 0;
}
