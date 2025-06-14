#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <log_file_path>\n", argv[0]);
        return 1;
    }

    FILE *file;
    char line[MAX_LINE_LENGTH];
    int warning_count = 0, error_count = 0;
    clock_t start, end;
    double cpu_time_used;

    // Start measuring time
    start = clock();

    file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "WARNING")) {
            warning_count++;
        }
        if (strstr(line, "ERROR")) {
            error_count++;
        }
    }

    fclose(file);

    // End measuring time
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("WARNING lines: %d\n", warning_count);
    printf("ERROR lines: %d\n", error_count);
    printf("Execution time: %.4f seconds\n", cpu_time_used);

    return 0;
}
