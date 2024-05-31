#include <alsa/asoundlib.h>

long int get_current_volume_level() {
	long int currVolumeLeft;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);

	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &currVolumeLeft);
	snd_mixer_close(handle);
	
	return currVolumeLeft;
}

/* mode can be volume up or volume down e.g: change_volume_level("volume down"); */
void change_volume_level(const char *mode) {
	long int min;
	long int max;
	long int volume;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);

	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

	/// adjust volume level on mouse wheel
	volume = get_current_volume_level();

	/// preventing over max volume
	if (volume < min) {
		volume = min;
	}
	else if (volume > max) {
		volume = max;
	}

	if (strcmp(mode, "volume up") == 0) {
		snd_mixer_selem_set_playback_volume_all(elem, (volume + 3));
	}
	else {
		snd_mixer_selem_set_playback_volume_all(elem, (volume - 3));
	}

	snd_mixer_close(handle);
}

/* Set volume level to given value */
void set_volume_level(int volumeLevel) {
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);

	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);
	snd_mixer_selem_set_playback_volume_all(elem, volumeLevel);
	snd_mixer_close(handle);
}

/* Get maximum supported volume level */
long int get_max_volume_level() {
	long int min;
	long int max;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);

	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_close(handle);

	return max;
}
