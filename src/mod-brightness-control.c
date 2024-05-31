#include <stdio.h>

/* brightness control */

int get_current_brightness(const char *path) {
	int brightness = -1;
	FILE *brightness_file = fopen(path, "r");
	if (brightness_file != NULL) {
		fscanf(brightness_file, "%d", &brightness);
		fclose(brightness_file);
	}
    else {
		printf("Error opening brightness file.\n");
	}
	return brightness;
}

void set_brightness(int level, const char *path) {
	FILE *brightness_file = fopen(path, "w");
	if (brightness_file != NULL) {
		fprintf(brightness_file, "%d", level);
		fclose(brightness_file);
	}
    else {
		printf("Error opening brightness file.\n");
	}
}
