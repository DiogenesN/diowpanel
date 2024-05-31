/******* runs a command and stores it's output into a char array
 *example:
	char *buffer[MAX_LINES];
	const char *command = "nmcli --fields SSID device wifi";
	output_to_char_array(buffer, command);
	printf("buffer[1]: %s\n", buffer[1]);
 *don't forget to free the memory:
	for (int i = 0; i < MAX_LINES && buffer[i] != NULL; i++) {
		free(buffer[i]);
		buffer[i] = NULL;
	}
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 4096
#define MAX_LINES 300

char *prerun_cmd_arr(const char *command) {
	char buffer[MAX_BUFFER_SIZE];
	char *result = NULL;
	FILE *fp = popen(command, "r");

	if (fp == NULL) {
		perror("popen");
		return NULL;
	}

	size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
	if (bytesRead > 0) {
		buffer[bytesRead] = '\0'; // Null-terminate the string
		result = strdup(buffer);
	}

	pclose(fp);
	return result;
}

char **output_to_char_array(char *buffer[], const char *command) {
	char *output = prerun_cmd_arr(command);

	if (output != NULL) {
		char *line = strtok(output, "\n");
		int lineCount = 0;

		while (line != NULL && lineCount < MAX_LINES) {
			buffer[lineCount] = strdup(line);
			line = strtok(NULL, "\n");
			lineCount++;
		}
	buffer[lineCount] = NULL;
	}
	free(output);
	return buffer;
}
