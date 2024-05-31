/* Return the nth (starting with 0) line from a text file
 * usage:
 * char *path = "/path/to/file.txt";
 * int line_number = 2; // Example: Get the second line
 * char *line = returnlinefromfile(path, line_number);
 * Don't forget to free the memory:
 * free(line); // Free the allocated memory
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *returnlinefromfile(char *path, int line) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", path);
        return NULL;
    }

    char *buffer = NULL;
    size_t bufsize = 0;
    int current_line = 0;

    // Read the file line by line until reaching the desired line
    while (getline(&buffer, &bufsize, file) != -1) {
        if (current_line == line) {
            // Remove newline character if it exists
            size_t length = strlen(buffer);
            if (length > 0 && buffer[length - 1] == '\n') {
                buffer[length - 1] = '\0';
            }
            fclose(file);
            return buffer;
        }
        current_line = current_line + 1;
    }

    // Close the file and free memory if line not found
    fclose(file);
    free(buffer);
    return NULL;
}
