# DioWPanel
A Wayland application that represents a little panel with widgets. If you like it and you want some changes and customization then feel free to add any changes you want or email me..
Scrolling anywhere on the panel changes the volume level. Right click on the panel terminates the panel.
It was tested on Debian 12 on Wayfire.

# What you can do with DioWWindowList
   1. Adjust volume level.
   2. Adjust bightness level.
   3. Showing a digital clock and calendar with the current month..
   4. Showing the current keyboard layout (at the moment there is no option to change layouts).
   5. Quick notes widget.
   6. Browse/connect/disconnect to wifi.

   also you need to install the following libs:

		sed
		grep
		make
		pkgconf
		awk (gawk)
	 	librsvg2-dev
		libcairo2-dev
		libwayland-dev
		libasound2-dev
		network-manager
		libxkbcommon-dev
		libc6-dev (spawn.h)

   on Debian run the following command:

		sudo apt install sed grep make pkgconf awk librsvg2-dev libcairo2-dev libwayland-dev libasound2-dev network-manager libxkbcommon-dev libc6-dev

   after going through all the above steps, go to the next step in this tutorial.

# Installation/Usage
  1. Open a terminal and run:

		 chmod +x ./configure
		 ./configure

  3. if all went well then run:

		 make
		 sudo make install
		 
		 (if you just want to test it then run: make run)
		
  4. Run the application:
  
		 diowpanel

# Configuration
The application creates the following configuration file

		~/.config/diowpanel/diowpanel.conf

   the default values are:

		volume_position_x=0
		volume_position_y=0
		brightness_file_path=/sys/class/backlight/intel_backlight/brightness
		keyboard_indicator=true
		enable_wifi=/usr/bin/nmcli radio wifi on
		disable_wifi=/usr/bin/nmcli radio wifi off
		wifi_state=nmcli radio wifi
		check_connectivity=nmcli networking connectivity check
		check_if_saved=nmcli connection show | grep -w -o
		current_active_connection=nmcli --field=NAME connection show --active | awk 'NR==2'
		network_scan=/usr/bin/nmcli --fields SSID device wifi | sed '1d' | awk '{$1=$1;print}'
		network_key_scan=/usr/bin/nmcli --fields=SECURITY device wifi | sed '1d' | awk '{$1=$1;print}' | sed 's/--/free/g'
		cmd_connect=/usr/bin/nmcli device wifi connect
		disconnect_cmd=/usr/bin/nmcli connection down id
		create_hotspot=/usr/bin/nmcli connection add type wifi con-name quick-hotspot autoconnect no wifi.mode ap wifi.ssid quick-hotspot ipv4.method shared ipv6.method shared
		activate_hotspot=/usr/bin/nmcli con up quick-hotspot
		nm_manager=/usr/bin/nm-connection-editor

   if you want to use your own commands, this is what you have to consider:

		volume_position. You only need to set the volume popup position
		All other widgets use volume position as an entry point, example:
		volume_position_x=-1860
		volume_position_y=-800
		brightness_file_path. Specify the path to the file responssible for brightness control
		keyboard_indicator. Either true or false, showing current keyboard layout, might break on some compositors, set if to false if it breaks
		Notes popup usage: by default a single click on any line, copies it to clipboard
		Adding a new note appends it to the end of file
		Paste Here, pastes clipboard content to the end of file
		To remove a line click on Remove and select the line you want to remove
		Currently notes doesn't support typing multibyte characters
		enable_wifi/disable_wifi, it's important to provide the full path to executable
		wifi_state, check whether wifi is enabled, the command must return "enabled" if wifi radio is enabled
		check_connectivity command must return "full" if there is an active connection
		check_if_saved command "nmcli connection show | grep -w -o" expects the network name as input, you must not provide the network name here, it is automatically added when you click on 'connect' (e.g. nmcli connection show | grep -w -o "MATRIX") and if it returns MATRUX, this network has been saved before and it doesn't need a password prompt to connect to, if you didn't mark it as 'connect automatically' this might be a problem
		current_active_connection command should return only one single string containing the curently active connection (e.g. MATRIX)")
		network_scan must return just available SSIDs and nothing more, you need to provide the full path to executable (e.g. /usr/bin/nmcli)"
		network_key_scan must return the security state of the available networks (aka either password peotected or free) if the network is free then make sure the command explicitely returns the word 'free', you must provide the full path to the executable (e.g. /usr/bin/nmcli)"
		cmd_connect or disconnect_cmd connects/disconnects the currently selected/active network, it takes the network name as input (e.g. nmcli device wifi connect "MATRIX" or nmcli connection down id "MATRIX") you must not provide the network name, just the command, also you must provide the full path to the nmcli execulable"
		create_hotspot must have SSID 'quick-hotspot' and must not be password protected
		activate_hotspot, hotspot SSID must be quick-hotspot

   any change in the configuration file requires application restart, right click on the panel will close the panel after that launch it again.

# Screenshots

![Alt text](https://raw.githubusercontent.com/DiogenesN/diowpanel/main/diowpanel-volume.png)

![Alt text](https://raw.githubusercontent.com/DiogenesN/diowpanel/main/diowpanel-calendar.png)

![Alt text](https://raw.githubusercontent.com/DiogenesN/diowpanel/main/diowpanel-notes.png)

![Alt text](https://raw.githubusercontent.com/DiogenesN/diowpanel/main/diowpanel-notwork.png)

That's it!

# Support

   My Libera IRC support channel: #linuxfriends

   Matrix: https://matrix.to/#/#linuxfriends2:matrix.org

   Email: nicolas.dio@protonmail.com
