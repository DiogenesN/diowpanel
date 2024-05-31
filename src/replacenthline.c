#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void replacenthline(const char *filename, int n, const char *newContent) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}

	char buffer[4096];
	FILE *tempFile = tmpfile();

	int currentLine = 0;
	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		currentLine++;
		if (currentLine == n) {
			// If the newContent doesn't end with a newline, add it
			if (newContent[strlen(newContent) - 1] != '\n') {
				fputs(newContent, tempFile);
				fputc('\n', tempFile);  // Add a newline character
			}
			else {
				fputs(newContent, tempFile);
			}
		}
		else {
			fputs(buffer, tempFile);
		}
	}

	// Close the original file and re-open it for writing
	fclose(file);
	file = fopen(filename, "w");

	// Copy the modified content back to the original file
	rewind(tempFile);
	while (fgets(buffer, sizeof(buffer), tempFile) != NULL) {
		fputs(buffer, file);
	}

	fclose(file);
	fclose(tempFile);
}
