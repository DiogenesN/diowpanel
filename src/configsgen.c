// SPDX-License-Identifier: GPL-2.0-or-later

/* creates initial config directory and file */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "runcmd.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cairo/cairo-svg.h>

void create_configs() {
	const char *HOME = getenv("HOME");
	if (HOME == NULL) {
		fprintf(stderr, "Unable to determine the user's home directory.\n");
		return;
	}
	const char *dirConfig		= "/.config/diowpanel";
	const char *fileConfig		= "/diowpanel.conf";
	const char *notesTXT		= "/notes.txt";
	const char *noicon			= "/noicon.svg";
	const char *dioVolHigh		= "/dio-volume-high.svg";
	const char *dioVolMid		= "/dio-volume-mid.svg";
	const char *dioVolLow		= "/dio-volume-low.svg";
	const char *dioVolOff		= "/dio-volume-off.svg";
	const char *dioBrightness	= "/brightness.svg";
	const char *dioNotes		= "/notes.svg";
	const char *dioNetwork		= "/network.svg";
	const char *dioNetworkOff	= "/network-off.svg";

	char dirConfigBuff[strlen(HOME) + strlen(dirConfig) + 3];
	char fileConfigBuff[strlen(HOME) + strlen(dirConfig) + strlen(fileConfig) + 3];
	char notesTXTBuff[strlen(HOME) + strlen(dirConfig) + strlen(notesTXT) + 3];
	char noiconBuff[strlen(HOME) + strlen(dirConfig) + strlen(noicon) + 3];
	char dioVolHighBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioVolHigh) + 3];
	char dioVolMidBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioVolMid) + 3];
	char dioVolLowBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioVolLow) + 3];
	char dioVolOffBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioVolOff) + 3];
	char dioBrightnessBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioBrightness) + 3];
	char dioNotesBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioNotes) + 3];
	char dioNetworkBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioNetwork) + 3];
	char dioNetworkOffBuff[strlen(HOME) + strlen(dirConfig) + strlen(dioNetworkOff) + 3];

	snprintf(dirConfigBuff, sizeof(dirConfigBuff), "%s%s", HOME, dirConfig);
	snprintf(fileConfigBuff, sizeof(fileConfigBuff), "%s%s%s", HOME, dirConfig, fileConfig);
	snprintf(notesTXTBuff, sizeof(notesTXTBuff), "%s%s%s", HOME, dirConfig, notesTXT);
	snprintf(noiconBuff, sizeof(noiconBuff), "%s%s%s", HOME, dirConfig, noicon);
	snprintf(dioVolHighBuff, sizeof(dioVolHighBuff), "%s%s%s", HOME, dirConfig, dioVolHigh);
	snprintf(dioVolMidBuff, sizeof(dioVolMidBuff), "%s%s%s", HOME, dirConfig, dioVolMid);
	snprintf(dioVolLowBuff, sizeof(dioVolLowBuff), "%s%s%s", HOME, dirConfig, dioVolLow);
	snprintf(dioVolOffBuff, sizeof(dioVolOffBuff), "%s%s%s", HOME, dirConfig, dioVolOff);
	snprintf(dioBrightnessBuff, sizeof(dioBrightnessBuff), "%s%s%s", HOME, dirConfig, dioBrightness);
	snprintf(dioNotesBuff, sizeof(dioNotesBuff), "%s%s%s", HOME, dirConfig, dioNotes);
	snprintf(dioNetworkBuff, sizeof(dioNetworkBuff), "%s%s%s", HOME, dirConfig, dioNetwork);
	snprintf(dioNetworkOffBuff, sizeof(dioNetworkOffBuff), "%s%s%s", HOME, dirConfig, dioNetworkOff);

	DIR *confDir = opendir(dirConfigBuff);
	struct stat buffer;
	// cheks if the file already exists
	if (confDir && stat(dirConfigBuff, &buffer) == 0) {
		// directory exists nothing to do
		printf("Configs exist, nothing to do!\n");
		closedir(confDir);
		return;
	}
	else {
		/// creating directory
		mkdir(dirConfigBuff, 0755);
		closedir(confDir);
		/// creating config file
		FILE *config = fopen(fileConfigBuff, "w+");
		fprintf(config, "%s\n", "volume_position_x=0");
		fprintf(config, "%s\n", "volume_position_y=0");
		fprintf(config, "%s\n", "brightness_file_path=/sys/class/backlight/intel_backlight/brightness");
		fprintf(config, "%s\n", "keyboard_indicator=true");
		fprintf(config, "%s\n", "# Network Management");
		fprintf(config, "%s\n", "enable_wifi=/usr/bin/nmcli radio wifi on");
		fprintf(config, "%s\n", "disable_wifi=/usr/bin/nmcli radio wifi off");
		fprintf(config, "%s\n", "# for the above commands it's important to provide the full path to executable");
		fprintf(config, "%s\n", "wifi_state=nmcli radio wifi");
		fprintf(config, "%s\n", "# check whether wifi is enabled, the command must return \"enabled\" if wifi radio is enabled");
		fprintf(config, "%s\n", "check_connectivity=nmcli networking connectivity check");
		fprintf(config, "%s\n", "# check_connectivity command must return \"full\" if there is an active connection");
		fprintf(config, "%s\n", "check_if_saved=nmcli connection show | grep -w -o");
		fprintf(config, "%s\n", "# check_if_saved command \"nmcli connection show | grep -w -o\" expects the network name as input, you must not provide the network name here, it is automatically added when you click on 'connect' (e.g. nmcli connection show | grep -w -o \"MATRIX\"\" and if it returns MATRUX, this network has been saved before and it doesn't need a password prompt to connect to, if you didn't mark it as 'connect automatically' this might be a problem");
		fprintf(config, "%s\n", "current_active_connection=nmcli --field=NAME connection show --active | awk 'NR==2'");
		fprintf(config, "%s\n", "# current_active_connection command should return only one single string containint the curently active connection (e.g. MATRIX)");
		fprintf(config, "%s\n", "network_scan=/usr/bin/nmcli --fields SSID device wifi | sed '1d' | awk '{$1=$1;print}'");
		fprintf(config, "%s\n", "# network_scan must return just available SSIDs and nothing more, you need to provide the full path to executable (e.g. /usr/bin/nmcli)");
		fprintf(config, "%s\n", "network_key_scan=/usr/bin/nmcli --fields=SECURITY device wifi | sed '1d' | awk '{$1=$1;print}' | sed 's/--/free/g'");
		fprintf(config, "%s\n", "# network_key_scan must return the security state of the available networks (aka either password peotected or free) if the network is free then make sure the command explicitely returns the word 'free', you must provide the full path to the executable (e.g. /usr/bin/nmcli)");
		fprintf(config, "%s\n", "cmd_connect=/usr/bin/nmcli device wifi connect");
		fprintf(config, "%s\n", "disconnect_cmd=/usr/bin/nmcli connection down id");
		fprintf(config, "%s\n", "# cmd_connect or disconnect_cmd connects/disconnects the currently selected/active network, it takes the network name as input (e.g. nmcli device wifi connect \"MATRIX\" or nmcli connection down id \"MATRIX\") you must not provide the network name, just the command, also you must provide the full path to the nmcli execulable");
		fprintf(config, "%s\n", "create_hotspot=/usr/bin/nmcli connection add type wifi con-name quick-hotspot autoconnect no wifi.mode ap wifi.ssid quick-hotspot ipv4.method shared ipv6.method shared");
		fprintf(config, "%s\n", "# create_hotspot must have SSID 'quick-hotspot' and must not be password protected");
		fprintf(config, "%s\n", "activate_hotspot=/usr/bin/nmcli con up quick-hotspot");
		fprintf(config, "%s\n", "# activate_hotspot, hotspot SSID must be quick-hotspot");
		fprintf(config, "%s\n", "nm_manager=/usr/bin/nm-connection-editor");
		fprintf(config, "\n%s\n", "# NOTE: Any changes here require app restart!");
		fprintf(config, "%s\n", "# volume_position. You only need to set the volume popup position.");
		fprintf(config, "%s\n", "# All other widgets use volume position as an entry point, example:");
		fprintf(config, "%s\n", "# volume_position_x=-1860");
		fprintf(config, "%s\n", "# volume_position_y=-800");
		fprintf(config, "%s\n", "# brightness_file_path. Specify the path to the file responssible for brightness control.");
		fprintf(config, "%s\n", "# keyboard_indicator. Either true or false, showing current keyboard layout, might break on some compositors, set if to false if it breaks.");
		fprintf(config, "%s\n", "# Notes popup usage: by default a single click on any line, copies it to clipboard");
		fprintf(config, "%s\n", "# Adding a new note appends it to the end of file");
		fprintf(config, "%s\n", "# Paste Here, pastes clipboard content to the end of file");
		fprintf(config, "%s\n", "# To remove a line click on Remove and select the line you want to remove");
		fprintf(config, "%s\n", "# Currently notes doesn't support typing multibyte characters.");
		fclose(config);

		/// notes.txt
		FILE *notes = fopen(notesTXTBuff, "w+");
		fprintf(notes, "%s\n", "Notes:\n");
		fclose(notes);

		//////////////////////////////// drawing volume off icon //////////////////////////
		cairo_surface_t *surfaceVolOff = cairo_svg_surface_create(dioVolOffBuff, 100, 100);
		cairo_t *crVolOff = cairo_create(surfaceVolOff);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crVolOff, 0.7, 0.7, 0.7); // Set source color to white
		cairo_arc(crVolOff, 50, 50, 30, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crVolOff); // Fill the circle with the current source color (white)
		/// deviding the circle in half vertically
		cairo_set_operator(crVolOff, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolOff, 5);
		cairo_move_to(crVolOff, 50, 20);
		cairo_line_to(crVolOff, 50, 80);
		cairo_stroke(crVolOff);
		/// draw circle 1 inside the initial circle
		cairo_set_line_width(crVolOff, 5);
		cairo_arc(crVolOff, 50, 50, 12, 0, 2 * M_PI);
		cairo_stroke(crVolOff);
		/// draw circle 2 inside the initial circle
		cairo_set_line_width(crVolOff, 7);
		cairo_arc(crVolOff, 50, 50, 22, 0, 2 * M_PI);
		cairo_stroke(crVolOff);
		/// curve to remove the last circle sound wave
		cairo_set_line_width(crVolOff, 24);
		cairo_curve_to(crVolOff, 50, 19, 102, 50, 50, 81);
		cairo_stroke(crVolOff);
		/// removing last wave for volume off icon
		cairo_set_line_width(crVolOff, 10);
		cairo_move_to(crVolOff, 55, 22);
		cairo_line_to(crVolOff, 55, 79);
		cairo_stroke(crVolOff);
		/// painiting/covering out unneeded transparent lines on the left side of the circle
		cairo_set_operator(crVolOff, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolOff, 5);
		cairo_move_to(crVolOff, 45, 22);
		cairo_line_to(crVolOff, 45, 79);
		cairo_stroke(crVolOff);
		cairo_set_line_width(crVolOff, 13);
		cairo_move_to(crVolOff, 39, 25);
		cairo_line_to(crVolOff, 39, 75);
		cairo_stroke(crVolOff);
		cairo_set_line_width(crVolOff, 12);
		cairo_move_to(crVolOff, 33, 30);
		cairo_line_to(crVolOff, 33, 70);
		cairo_stroke(crVolOff);
		cairo_set_line_width(crVolOff, 10);
		cairo_move_to(crVolOff, 27, 40);
		cairo_line_to(crVolOff, 27, 60);
		cairo_stroke(crVolOff);
		/// cutting space before adding a tail
		cairo_set_operator(crVolOff, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolOff, 17);
		cairo_move_to(crVolOff, 15, 20);
		cairo_line_to(crVolOff, 15, 80);
		cairo_stroke(crVolOff);
		/// adding a tail at the end of volume icon
		cairo_set_operator(crVolOff, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolOff, 10);
		cairo_move_to(crVolOff, 15, 38);
		cairo_line_to(crVolOff, 15, 62);
		cairo_stroke(crVolOff);

		//////////////////////////////// drawing volume low icon //////////////////////////
		cairo_surface_t *surfaceVolLow = cairo_svg_surface_create(dioVolLowBuff, 100, 100);
		cairo_t *crVolLow = cairo_create(surfaceVolLow);

		/// draw initial circle filled inside
		cairo_set_source_rgb(crVolLow, 0.9, 0.9, 0.8); // Set source color to white
		cairo_arc(crVolLow, 50, 50, 30, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crVolLow); // Fill the circle with the current source color (white)
		/// deviding the circle in half vertically
		cairo_set_operator(crVolLow, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolLow, 5);
		cairo_move_to(crVolLow, 50, 20);
		cairo_line_to(crVolLow, 50, 80);
		cairo_stroke(crVolLow);
		/// draw circle 1 inside the initial circle
		cairo_set_line_width(crVolLow, 5);
		cairo_arc(crVolLow, 50, 50, 12, 0, 2 * M_PI);
		cairo_stroke(crVolLow);
		/// draw circle 2 inside the initial circle
		cairo_set_line_width(crVolLow, 7);
		cairo_arc(crVolLow, 50, 50, 22, 0, 2 * M_PI);
		cairo_stroke(crVolLow);
		/// curve to remove the last circle sound wave
		cairo_set_line_width(crVolLow, 24);
		cairo_curve_to(crVolLow, 50, 19, 102, 50, 50, 81);
		cairo_stroke(crVolLow);
		/// painiting/covering out unneeded transparent lines on the left side of the circle
		cairo_set_operator(crVolLow, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolLow, 5);
		cairo_move_to(crVolLow, 45, 22);
		cairo_line_to(crVolLow, 45, 79);
		cairo_stroke(crVolLow);
		cairo_set_line_width(crVolLow, 13);
		cairo_move_to(crVolLow, 39, 25);
		cairo_line_to(crVolLow, 39, 75);
		cairo_stroke(crVolLow);
		cairo_set_line_width(crVolLow, 12);
		cairo_move_to(crVolLow, 33, 30);
		cairo_line_to(crVolLow, 33, 70);
		cairo_stroke(crVolLow);
		cairo_set_line_width(crVolLow, 10);
		cairo_move_to(crVolLow, 27, 40);
		cairo_line_to(crVolLow, 27, 60);
		cairo_stroke(crVolLow);
		/// cutting space before adding a tail
		cairo_set_operator(crVolLow, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolLow, 17);
		cairo_move_to(crVolLow, 15, 20);
		cairo_line_to(crVolLow, 15, 80);
		cairo_stroke(crVolLow);
		/// adding a tail at the end of volume icon
		cairo_set_operator(crVolLow, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolLow, 10);
		cairo_move_to(crVolLow, 15, 38);
		cairo_line_to(crVolLow, 15, 62);
		cairo_stroke(crVolLow);

		//////////////////////////////// drawing volume mid icon //////////////////////////
		cairo_surface_t *surfaceVolMid = cairo_svg_surface_create(dioVolMidBuff, 100, 100);
		cairo_t *crVolMid = cairo_create(surfaceVolMid);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crVolMid, 0.9, 0.9, 0.8); // Set source color to white
		cairo_arc(crVolMid, 50, 50, 30, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crVolMid); // Fill the circle with the current source color (white)
		/// deviding the circle in half vertically
		cairo_set_operator(crVolMid, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolMid, 5);
		cairo_move_to(crVolMid, 50, 20);
		cairo_line_to(crVolMid, 50, 80);
		cairo_stroke(crVolMid);
		/// draw circle 1 inside the initial circle
		cairo_set_line_width(crVolMid, 5);
		cairo_arc(crVolMid, 50, 50, 12, 0, 2 * M_PI);
		cairo_stroke(crVolMid);
		/// draw circle 2 inside the initial circle
		cairo_set_line_width(crVolMid, 7);
		cairo_arc(crVolMid, 50, 50, 22, 0, 2 * M_PI);
		cairo_stroke(crVolMid);
		/// curve to remove the last circle sound wave
		cairo_set_line_width(crVolMid, 10);
		cairo_curve_to(crVolMid, 54, 19, 102, 50, 54, 81);
		cairo_stroke(crVolMid);
		/// painiting/covering out unneeded transparent lines on the left side of the circle
		cairo_set_operator(crVolMid, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolMid, 5);
		cairo_move_to(crVolMid, 45, 25);
		cairo_line_to(crVolMid, 45, 75);
		cairo_stroke(crVolMid);
		cairo_set_line_width(crVolMid, 13);
		cairo_move_to(crVolMid, 39, 25);
		cairo_line_to(crVolMid, 39, 75);
		cairo_stroke(crVolMid);
		cairo_set_line_width(crVolMid, 12);
		cairo_move_to(crVolMid, 33, 30);
		cairo_line_to(crVolMid, 33, 70);
		cairo_stroke(crVolMid);
		cairo_set_line_width(crVolMid, 10);
		cairo_move_to(crVolMid, 27, 40);
		cairo_line_to(crVolMid, 27, 60);
		cairo_stroke(crVolMid);
		/// cutting space before adding a tail
		cairo_set_operator(crVolMid, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolMid, 17);
		cairo_move_to(crVolMid, 15, 20);
		cairo_line_to(crVolMid, 15, 80);
		cairo_stroke(crVolMid);
		/// adding a tail at the end of volume icon
		cairo_set_operator(crVolMid, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolMid, 10);
		cairo_move_to(crVolMid, 15, 38);
		cairo_line_to(crVolMid, 15, 62);
		cairo_stroke(crVolMid);

		//////////////////////////////// drawing volume max icon //////////////////////////
		cairo_surface_t *surfaceVolHigh = cairo_svg_surface_create(dioVolHighBuff, 100, 100);
		cairo_t *crVolHigh = cairo_create(surfaceVolHigh);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crVolHigh, 0.9, 0.9, 0.8); // Set source color to white
		cairo_arc(crVolHigh, 50, 50, 30, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crVolHigh); // Fill the circle with the current source color (white)
		/// deviding the circle in half vertically
		cairo_set_operator(crVolHigh, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolHigh, 5);
		cairo_move_to(crVolHigh, 50, 20);
		cairo_line_to(crVolHigh, 50, 80);
		cairo_stroke(crVolHigh);
		/// draw circle 1 inside the initial circle
		cairo_set_line_width(crVolHigh, 5);
		cairo_arc(crVolHigh, 50, 50, 12, 0, 2 * M_PI);
		cairo_stroke(crVolHigh);
		/// draw circle 2 inside the initial circle
		cairo_set_line_width(crVolHigh, 5);
		cairo_arc(crVolHigh, 50, 50, 22, 0, 2 * M_PI);
		cairo_stroke(crVolHigh);
		/// painiting/covering out unneeded transparent lines on the left side of the circle
		cairo_set_operator(crVolHigh, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolHigh, 5);
		cairo_move_to(crVolHigh, 45, 25);
		cairo_line_to(crVolHigh, 45, 75);
		cairo_stroke(crVolHigh);
		cairo_set_line_width(crVolHigh, 13);
		cairo_move_to(crVolHigh, 39, 25);
		cairo_line_to(crVolHigh, 39, 75);
		cairo_stroke(crVolHigh);
		cairo_set_line_width(crVolHigh, 10);
		cairo_move_to(crVolHigh, 33, 30);
		cairo_line_to(crVolHigh, 33, 70);
		cairo_stroke(crVolHigh);
		cairo_set_line_width(crVolHigh, 10);
		cairo_move_to(crVolHigh, 27, 40);
		cairo_line_to(crVolHigh, 27, 60);
		cairo_stroke(crVolHigh);
		/// cutting space before adding a tail
		cairo_set_operator(crVolHigh, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crVolHigh, 17);
		cairo_move_to(crVolHigh, 15, 20);
		cairo_line_to(crVolHigh, 15, 80);
		cairo_stroke(crVolHigh);
		/// adding a tail at the end of volume icon
		cairo_set_operator(crVolHigh, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crVolHigh, 10);
		cairo_move_to(crVolHigh, 15, 38);
		cairo_line_to(crVolHigh, 15, 62);
		cairo_stroke(crVolHigh);

		/////////////////////////////// drawing bightness icon ////////////////////////////
		cairo_surface_t *surfaceBright = cairo_svg_surface_create(dioBrightnessBuff, 100, 100);
		cairo_t *crBright = cairo_create(surfaceBright);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crBright, 0.9, 0.9, 0.9); // Set source color to white
		cairo_arc(crBright, 50, 50, 30, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crBright); // Fill the circle with the current source color (white)
		/// draw circle 1 inside the initial circle
		cairo_set_operator(crBright, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crBright, 5);
		cairo_arc(crBright, 50, 50, 20, 0, 2 * M_PI);
		cairo_stroke(crBright);
		/// upper line 1
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 42, 30);
		cairo_line_to(crBright, 42, 0);
		cairo_stroke(crBright);
		/// upper line 2
		cairo_set_line_width(crBright, 3);
		cairo_move_to(crBright, 17, -85);
		cairo_line_to(crBright, 39, 35);
		cairo_stroke(crBright);
		/// upper line 2
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 58, 30);
		cairo_line_to(crBright, 58, 0);
		cairo_stroke(crBright);
		/// upper line 3
		cairo_set_line_width(crBright, 5);
		cairo_move_to(crBright, 70, 10);
		cairo_line_to(crBright, 60, 30);
		cairo_stroke(crBright);
		/// upper line 4
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 97, 27);
		cairo_line_to(crBright, 67, 40);
		cairo_stroke(crBright);
		/// upper line 5
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 97, 35);
		cairo_line_to(crBright, 67, 40);
		cairo_stroke(crBright);
		/// lower line 1
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 97, 57);
		cairo_line_to(crBright, 67, 57);
		cairo_stroke(crBright);
		/// lower line 2
		cairo_set_line_width(crBright, 11);
		cairo_move_to(crBright, 97, 70);
		cairo_line_to(crBright, 67, 55);
		cairo_stroke(crBright);
		/// lower line 3
		cairo_set_line_width(crBright, 5);
		cairo_move_to(crBright, 60, 70);
		cairo_line_to(crBright, 69, 90);
		cairo_stroke(crBright);
		/// lower line 4
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 58, 90);
		cairo_line_to(crBright, 58, 70);
		cairo_stroke(crBright);
		/// lower line 5
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 42, 90);
		cairo_line_to(crBright, 42, 70);
		cairo_stroke(crBright);
		/// lower line 6
		cairo_set_line_width(crBright, 4);
		cairo_move_to(crBright, 34, 81);
		cairo_line_to(crBright, 42, 67);
		cairo_stroke(crBright);
		/// lower line 7
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 5, 73);
		cairo_line_to(crBright, 30, 60);
		cairo_stroke(crBright);
		/// lower line 8
		cairo_set_line_width(crBright, 7);
		cairo_move_to(crBright, 5, 65);
		cairo_line_to(crBright, 30, 57);
		cairo_stroke(crBright);
		/// lower line 9
		cairo_set_line_width(crBright, 9);
		cairo_move_to(crBright, 5, 33);
		cairo_line_to(crBright, 33, 40);
		cairo_stroke(crBright);

		///////////////////////////////// drawing notes icon //////////////////////////////
		cairo_surface_t *surfaceNotes = cairo_svg_surface_create(dioNotesBuff, 100, 100);
		cairo_t *crNotes = cairo_create(surfaceNotes);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crNotes, 0.9, 0.9, 0.9); // Set source color to white
		/// draw notes
		cairo_set_line_width(crNotes, 7);
		cairo_rectangle (crNotes, 10, 15, 60, 65);
		cairo_stroke(crNotes);
		/// draw text line 1
		cairo_set_line_width(crNotes, 5);
		cairo_move_to(crNotes, 20, 30);
		cairo_line_to(crNotes, 60, 30);
		cairo_stroke(crNotes);
		/// draw text line 1
		cairo_set_line_width(crNotes, 5);
		cairo_move_to(crNotes, 20, 45);
		cairo_line_to(crNotes, 60, 45);
		cairo_stroke(crNotes);
		/// draw text line 3
		cairo_set_line_width(crNotes, 5);
		cairo_move_to(crNotes, 20, 60);
		cairo_line_to(crNotes, 60, 60);
		cairo_stroke(crNotes);
		/// draw pencil
		cairo_set_line_width(crNotes, 15);
		cairo_move_to(crNotes, 75, 35);
		cairo_line_to(crNotes, 55, 85);
		cairo_stroke(crNotes);
		/// draw pencil inside
		cairo_set_operator(crNotes, CAIRO_OPERATOR_CLEAR);
		cairo_set_line_width(crNotes, 6);
		cairo_move_to(crNotes, 73, 39);
		cairo_line_to(crNotes, 56, 83);
		cairo_stroke(crNotes);
		/// draw end of pencil 1
		cairo_set_operator(crNotes, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crNotes, 2);
		cairo_move_to(crNotes, 52, 81);
		cairo_line_to(crNotes, 51, 92);
		cairo_stroke(crNotes);
		/// draw end of pencil 2
		cairo_set_operator(crNotes, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(crNotes, 2);
		cairo_move_to(crNotes, 59, 85);
		cairo_line_to(crNotes, 51, 92);
		cairo_stroke(crNotes);

		/////////////////////////////// drawing nwtwork icon //////////////////////////////
		cairo_surface_t *surfaceNetwork = cairo_svg_surface_create(dioNetworkBuff, 100, 100);
		cairo_t *crNetwork = cairo_create(surfaceNetwork);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crNetwork, 0.9, 0.9, 0.9); // Set source color to white
		cairo_arc(crNetwork, 50, 73, 10, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crNetwork); // Fill the circle with the current source color (white)
		/// network wave 1
		cairo_set_line_width(crNetwork, 7);
		cairo_curve_to(crNetwork, 30, 63, 50, 35, 70, 63);
		cairo_stroke(crNetwork);
		/// network wave 2
		cairo_set_line_width(crNetwork, 7);
		cairo_curve_to(crNetwork, 20, 50, 50, 15, 80, 50);
		cairo_stroke(crNetwork);
		/// network wave 3
		cairo_set_line_width(crNetwork, 7);
		cairo_curve_to(crNetwork, 10, 35, 50, -5, 90, 35);
		cairo_stroke(crNetwork);

		//////////////////////////// drawing nwtwork off icon ////////////////////////////
		cairo_surface_t *surfaceNetOff = cairo_svg_surface_create(dioNetworkOffBuff, 100, 100);
		cairo_t *crNetOff = cairo_create(surfaceNetOff);
		/// draw initial circle filled inside
		cairo_set_source_rgb(crNetOff, 0.9, 0.9, 0.9); // Set source color to white
		cairo_arc(crNetOff, 50, 73, 10, 0, 2 * M_PI); // Draw the circle
		cairo_fill(crNetOff); // Fill the circle with the current source color (white)
		/// network wave 1
		cairo_set_line_width(crNetOff, 7);
		cairo_curve_to(crNetOff, 30, 63, 50, 35, 70, 63);
		cairo_stroke(crNetOff);
		/// network wave 2
		cairo_set_line_width(crNetOff, 7);
		cairo_curve_to(crNetOff, 20, 50, 50, 15, 80, 50);
		cairo_stroke(crNetOff);
		/// network wave 3
		cairo_set_line_width(crNetOff, 7);
		cairo_curve_to(crNetOff, 10, 35, 50, -5, 90, 35);
		cairo_stroke(crNetOff);
		/// red x for network off
		cairo_set_source_rgb(crNetOff, 1.0, 0.0, 0.0);
		cairo_set_line_width(crNetOff, 12);
		cairo_move_to(crNetOff, 30, 30);
		cairo_line_to(crNetOff, 70, 70);
		cairo_stroke(crNetOff);
		cairo_move_to(crNetOff, 70, 30);
		cairo_line_to(crNetOff, 30, 70);
		cairo_stroke(crNetOff);

		//////////////////////////// creating default noicon svg ////////////////////////////
		cairo_surface_t *surface = cairo_svg_surface_create(noiconBuff, 100, 100);
		cairo_t *cr = cairo_create(surface);
		// Clear the inside of the circle (transparent fill)
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0); // Transparent fill: RGBA(0, 0, 0, 0)
		cairo_fill(cr);
		// Draw the circle outline
		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // Red outline: RGB(1, 0, 0)
		cairo_set_line_width(cr, 10.0); // Set line width to 10 (adjust as needed)
		cairo_arc(cr, 50, 50, 40, 0, 2 * M_PI); // Center (50, 50), radius 40
		cairo_stroke(cr); // Draw the outline using stroke instead of fill
		// Draw the diagonal lines for close icon effect
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White line: RGB(1, 1, 1)
		cairo_set_line_width(cr, 5.0); // Set line width to 5 (adjust as needed)
		cairo_move_to(cr, 30, 30); // Move to the starting point of the line
		cairo_line_to(cr, 70, 70); // Draw a line to the ending point
		cairo_move_to(cr, 30, 70); // Move to another starting point
		cairo_line_to(cr, 70, 30); // Draw another line to another ending point
		cairo_stroke(cr); // Draw the lines using stroke

		/// clean up
		cairo_destroy(crVolOff);
		cairo_surface_destroy(surfaceVolOff);
		cairo_destroy(crVolLow);
		cairo_surface_destroy(surfaceVolLow);
		cairo_destroy(crVolMid);
		cairo_surface_destroy(surfaceVolMid);
		cairo_destroy(crVolHigh);
		cairo_surface_destroy(surfaceVolHigh);
		cairo_destroy(crBright);
		cairo_surface_destroy(surfaceBright);
		cairo_destroy(crNotes);
		cairo_surface_destroy(surfaceNotes);
		cairo_destroy(crNetwork);
		cairo_surface_destroy(surfaceNetwork);
		cairo_destroy(crNetOff);
		cairo_surface_destroy(surfaceNetOff);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);

		/// freeing resources
		printf("All config files created successfully!\n");
	}
	return;
}
