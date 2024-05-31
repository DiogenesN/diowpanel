/* Remove last character from the given file
 * Usage:
 * removelastcharfromfile("/path/to/file.txt");
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void removelastcharfromfile(char *path) {
	/// Open the file for reading and writing
	FILE *file = fopen(path, "r+");
	if (file == NULL) {
		perror("Error opening file");
		return;
	}

	/// Read the content of the file into a buffer
	char buffer[4096]; // Assuming a maximum line length of 255 characters
	if (fgets(buffer, sizeof(buffer), file) == NULL) {
		fprintf(stderr, "Error reading file\n");
		fclose(file);
		return;
	}

	/// Remove the last character from the string if it is not empty
	size_t length = strlen(buffer);
	if (length > 0) {
		buffer[length - 1] = '\0';
	}

	/// Move the file pointer to the beginning of the file
	rewind(file);

	/// Write the modified string back to the file
	fputs(buffer, file);

	/// Truncate the file to the current position (remove any remaining content)
	fflush(file);
	ftruncate(fileno(file), ftell(file));

	/// Close the file
	fclose(file);
}
