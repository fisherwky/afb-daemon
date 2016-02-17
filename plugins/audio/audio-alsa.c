/*
 * Copyright (C) 2015 "IoT.bzh"
 * Author "Manuel Bachmann"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "audio-api.h"

snd_mixer_selem_channel_id_t SCHANNELS[8] = {
 SND_MIXER_SCHN_FRONT_LEFT,
 SND_MIXER_SCHN_FRONT_RIGHT,
 SND_MIXER_SCHN_FRONT_CENTER,
 SND_MIXER_SCHN_REAR_LEFT,
 SND_MIXER_SCHN_REAR_RIGHT,
 SND_MIXER_SCHN_REAR_CENTER,
 SND_MIXER_SCHN_SIDE_LEFT,
 SND_MIXER_SCHN_SIDE_RIGHT
};

PUBLIC unsigned char _alsa_init (const char *name, audioCtxHandleT *ctx) {

    snd_pcm_t *dev;
    snd_pcm_hw_params_t *params;
    snd_mixer_t *mixer;
    snd_mixer_selem_id_t *mixer_sid;
    snd_mixer_elem_t *mixer_elm;
    snd_mixer_elem_t *mixer_elm_m;
    unsigned int rate = 22050;
    long vol, vol_min, vol_max;
    int num, i;

    if (snd_pcm_open (&dev, name, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return -1;

    snd_pcm_hw_params_malloc (&params);
    snd_pcm_hw_params_any (dev, params);
    snd_pcm_hw_params_set_access (dev, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format (dev, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near (dev, params, &rate, 0);
    snd_pcm_hw_params_set_channels (dev, params, ctx->channels);
    if (snd_pcm_hw_params (dev, params) < 0) {
        snd_pcm_hw_params_free (params);
        return -1;
    }
    snd_pcm_prepare (dev);

    snd_mixer_open (&mixer, 0);
    if (snd_mixer_attach (mixer, name) < 0) {
        snd_pcm_hw_params_free (params);
        return -1;
    }
    snd_mixer_selem_register (mixer, NULL, NULL);
    snd_mixer_load (mixer);

    snd_mixer_selem_id_alloca (&mixer_sid);
    snd_mixer_selem_id_set_index (mixer_sid, 0);
    snd_mixer_selem_id_set_name (mixer_sid, "Master");

    mixer_elm = snd_mixer_find_selem (mixer, mixer_sid);
    mixer_elm_m = NULL;

    if (!mixer_elm) {
        /* no "Master" mixer ; we are probably on a board... search ! */
        for (mixer_elm = snd_mixer_first_elem (mixer); mixer_elm != NULL;
             mixer_elm = snd_mixer_elem_next (mixer_elm)) {
            if (snd_mixer_elem_info (mixer_elm) < 0)
                continue;
            snd_mixer_selem_get_id (mixer_elm, mixer_sid);
            if (strstr (snd_mixer_selem_id_get_name (mixer_sid), "DVC Out")) {

                /* this is Porter... let us found the specific mute switch */
                snd_mixer_selem_id_set_index (mixer_sid, 0);
                snd_mixer_selem_id_set_name (mixer_sid, "DVC Out Mute");
                mixer_elm_m = snd_mixer_find_selem (mixer, mixer_sid);

                break;
            }
        }
    }

    if (mixer_elm) {
        snd_mixer_selem_get_playback_volume_range (mixer_elm, &vol_min, &vol_max);
        snd_mixer_selem_get_playback_volume (mixer_elm, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    }

    /* allocate the global array if it hasn't been done */
    if (!dev_ctx_a) {
        dev_ctx_a = (dev_ctx_alsa_T**) malloc (sizeof(dev_ctx_alsa_T));
        dev_ctx_a[0] = (dev_ctx_alsa_T*) malloc (sizeof(dev_ctx_alsa_T));
        dev_ctx_a[0]->name = NULL;
        dev_ctx_a[0]->dev = NULL;
    }

    /* is a card with similar name already opened ? */
    for (num = 0; num < (sizeof(dev_ctx_a)/sizeof(dev_ctx_alsa_T)); num++) {
        if (dev_ctx_a[num]->name &&
           !strcmp (dev_ctx_a[num]->name, name))
            return -1;
    }
    num++;

    /* it's not... let us add it to the global array */
    dev_ctx_a = (dev_ctx_alsa_T**) realloc (dev_ctx_a, (num+1)*sizeof(dev_ctx_alsa_T));
    dev_ctx_a[num] = (dev_ctx_alsa_T*) malloc (sizeof(dev_ctx_alsa_T));
    dev_ctx_a[num]->name = strdup (name);
    dev_ctx_a[num]->dev = dev;
    dev_ctx_a[num]->params = params;
    dev_ctx_a[num]->mixer_elm = mixer_elm;
    dev_ctx_a[num]->mixer_elm_m = mixer_elm_m;
    dev_ctx_a[num]->vol_max = vol_max;
    dev_ctx_a[num]->vol = vol;
    dev_ctx_a[num]->thr_should_run = 0;
    dev_ctx_a[num]->thr_finished = 0;

    /* make the client context aware of current card state */
    for (i = 0; i < 8; i++)
        ctx->volume[i] = _alsa_get_volume (num, i);
    ctx->mute = _alsa_get_mute (num);
    ctx->idx = num;

    if (verbose) fprintf (stderr, "Successfully initialized ALSA backend.\n");

    return 0;
}

PUBLIC void _alsa_free (const char *name) {

    int num;

    for (num = 0; num < (sizeof(dev_ctx_a)/sizeof(dev_ctx_alsa_T)); num++) {
        if (dev_ctx_a[num]->name &&
           !strcmp (dev_ctx_a[num]->name, name)) {
            snd_pcm_close (dev_ctx_a[num]->dev);
            snd_pcm_hw_params_free (dev_ctx_a[num]->params);
            free (dev_ctx_a[num]->name);
            dev_ctx_a[num]->dev = NULL;
            dev_ctx_a[num]->name = NULL;
            free (dev_ctx_a[num]);
            return;
        }
    }
}

PUBLIC void _alsa_play (unsigned int num) {

    if (!dev_ctx_a || !dev_ctx_a[num] || dev_ctx_a[num]->thr_should_run ||
        access (AUDIO_BUFFER, F_OK) == -1)
        return;

    dev_ctx_a[num]->thr_should_run = 1;
    dev_ctx_a[num]->thr_finished = 0;
    pthread_create (&dev_ctx_a[num]->thr, NULL, _alsa_play_thread_fn, (void*)dev_ctx_a[num]);
}

PUBLIC void _alsa_stop (unsigned int num) {

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->thr_should_run)
        return;

    /* stop the "while" loop in thread */
    dev_ctx_a[num]->thr_should_run = 0;

    while (!dev_ctx_a[num]->thr_finished)
        usleep(100000);

    pthread_join (dev_ctx_a[num]->thr, NULL);
}

PUBLIC int _alsa_get_volume (unsigned int num, unsigned int channel) {

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->mixer_elm)
        return;

    snd_mixer_selem_get_playback_volume (dev_ctx_a[num]->mixer_elm, SCHANNELS[channel], &dev_ctx_a[num]->vol);

    return (int)(dev_ctx_a[num]->vol*100)/dev_ctx_a[num]->vol_max;
}

PUBLIC void _alsa_set_volume (unsigned int num, unsigned int channel, int vol) {

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->mixer_elm ||
        0 > vol > 100)
        return;

    snd_mixer_selem_set_playback_volume (dev_ctx_a[num]->mixer_elm, SCHANNELS[channel], (vol*dev_ctx_a[num]->vol_max)/100);

}

PUBLIC void _alsa_set_volume_all (unsigned int num, int vol) {

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->mixer_elm ||
        0 > vol > 100)

    snd_mixer_selem_set_playback_volume_all (dev_ctx_a[num]->mixer_elm, (vol*dev_ctx_a[num]->vol_max)/100);
}

PUBLIC unsigned char _alsa_get_mute (unsigned int num) {

    int mute = 0;
    snd_mixer_elem_t *elm_m;

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->mixer_elm)
        return;

    dev_ctx_a[num]->mixer_elm_m ? (elm_m = dev_ctx_a[num]->mixer_elm_m) :
                                  (elm_m = dev_ctx_a[num]->mixer_elm);

    if (snd_mixer_selem_has_playback_switch (elm_m)) {
        snd_mixer_selem_get_playback_switch (elm_m, SND_MIXER_SCHN_FRONT_LEFT, &mute);
        snd_mixer_selem_get_playback_switch (elm_m, SND_MIXER_SCHN_FRONT_RIGHT, &mute);
    }

    if (dev_ctx_a[num]->mixer_elm_m)
        return (unsigned char)mute;
    else
        return (unsigned char)!mute;
}

PUBLIC void _alsa_set_mute (unsigned int num, unsigned char tomute) {

    snd_mixer_elem_t *elm_m;
    int mute;

    if (!dev_ctx_a || !dev_ctx_a[num] || !dev_ctx_a[num]->mixer_elm || 1 < tomute < 0)
        return;

    if (dev_ctx_a[num]->mixer_elm_m) {
        elm_m = dev_ctx_a[num]->mixer_elm_m;
        mute = (int)!tomute;
    } else {
        elm_m = dev_ctx_a[num]->mixer_elm;
        mute = (int)tomute;
    }

    if (snd_mixer_selem_has_playback_switch (elm_m))
        snd_mixer_selem_set_playback_switch_all (elm_m, !mute);
}

PUBLIC void _alsa_set_rate (unsigned int num, unsigned int rate) {

    if (!dev_ctx_a || !dev_ctx_a[num])
        return;

    snd_pcm_hw_params_set_rate_near (dev_ctx_a[num]->dev, dev_ctx_a[num]->params, &rate, 0);
}

PUBLIC void _alsa_set_channels (unsigned int num, unsigned int channels) {

    if (!dev_ctx_a || !dev_ctx_a[num])
        return;

    snd_pcm_hw_params_set_channels (dev_ctx_a[num]->dev, dev_ctx_a[num]->params, channels);
}

 /* ---- LOCAL THREADED FUNCTIONS ---- */

STATIC void* _alsa_play_thread_fn (void *ctx) {

    dev_ctx_alsa_T *dev_ctx_a = (dev_ctx_alsa_T *)ctx;
    FILE *file = NULL;
    char *buf = NULL;
    long size;
    int frames, res;

    file = fopen (AUDIO_BUFFER, "rb");

    while (dev_ctx_a->thr_should_run && file && (access (AUDIO_BUFFER, F_OK) != -1) ) {
        fseek (file, 0, SEEK_END);
        size = ftell (file);
        buf = (char*) realloc (buf, size * sizeof(char));
        frames = (size * sizeof(char)) / 4;

        fseek (file, 0, SEEK_SET);
        fread (buf, 1, size, file);
        fflush (file);

        if ((res = snd_pcm_writei (dev_ctx_a->dev, buf, frames)) != frames) {
            snd_pcm_recover (dev_ctx_a->dev, res, 0);
            snd_pcm_prepare (dev_ctx_a->dev);
        }
        /* snd_pcm_drain (dev_ctx->dev); */
    }
    if (buf) free(buf);
    if (file) fclose(file);

    dev_ctx_a->thr_finished = 1;
    return 0;
}
