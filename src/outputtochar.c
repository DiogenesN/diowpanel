/* run a command and returns its output that you can assing to a variable
 * you have to free the memory
 * example:
 *  const char *cmd = "pactl list sinks | grep 'Volume:' | head -n 1 | awk '{print $5}' | sed 's/%//'";
	const char *outputVol = output_to_char(cmd);
	free((void*)outputVol);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *output_to_char(const char *command) {
	FILE* fp = popen(command, "r");

	if (fp == NULL) {
		perror("popen");
		return NULL;
	}

	char buffer[777];

	size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
	if (bytesRead > 0) {
		// Remove trailing newline or whitespace characters
		while (bytesRead > 0 && (buffer[bytesRead - 1] == '\n' || buffer[bytesRead - 1] == ' ' || buffer[bytesRead - 1] == '\t')) {
			bytesRead--;
		}

		buffer[bytesRead] = '\0'; // Null-terminate the string
    }

	pclose(fp);

	// Duplicate the string before returning
	return strdup(buffer);
}
