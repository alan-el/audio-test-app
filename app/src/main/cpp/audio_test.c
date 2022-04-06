//
// Created by Alan on 2021/11/16.
//
#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include <android/log.h>
#include "include/tinyalsa/asoundlib.h"
#include "ndk_aaudio.h"

#define LOG_TAG "audio_test.c"

#define SOUND_DEV_IN_NONE           -1
#define SOUND_DEV_IN_ON_BOARD_MIC   0
#define SOUND_DEV_IN_HEADPHONE_MIC  1
#define SOUND_DEV_IN_USB_MIC        2
// TODO BLUETOOTH

#define SOUND_DEV_OUT_NONE          -1
#define SOUND_DEV_OUT_HP            10
#define SOUND_DEV_OUT_SPK           11
#define SOUND_DEV_OUT_USB_SPK       12
// TODO BLUETOOTH


#define RK809_IN_CONTROL        "1"
#define RK809_OUT_CONTROL       "0"

// mixer route name
const char *rn_in[] =
{
    "Main Mic",
    "Hands Free Mic",
    ""
};

const char *rn_out[] =
{
    "HP",
    "SPK",
    ""
};
/*-----------------------------------------------------------
#define PCM_STREAM_TYPE_IN  0
#define PCM_STREAM_TYPE_OUT 0

#define PHONE_SIDE_OUR      0
#define PHONE_SIDE_OPPOSITE 1

typedef struct phone_side
{
    // stream type
    int st;
    int card;
    int sub_dev;
    int is_open;
    struct pcm *ppcm;
}phone_side_t;

phone_side_t our_side = {PCM_STREAM_TYPE_IN, -1, 0, 0, NULL};
phone_side_t opst_side = {PCM_STREAM_TYPE_OUT, -1, 0, 0, NULL};
---------------------------------------------------------------------------------*/
typedef struct
{
    int device;
    int pcmC;
    int pcmD;

}sound_dev_t;

typedef struct
{
    sound_dev_t in;
    sound_dev_t out;
}cur_devices_t;

cur_devices_t cur_dev =
{
    {
            SOUND_DEV_IN_NONE,
            -1,
            -1
        },

    {
            SOUND_DEV_OUT_NONE,
            -1,
            -1
        }
};

// On board sound device name
static const char *obsd = "rockchip,rk809-codec\0";
// extend USB sound device name
static const char *eusd = "USB PnP Sound Device\0";

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static struct pcm *g_pcm[4] = {NULL, NULL, NULL, NULL};
static struct pcm_config g_config[4] = {0};

static void tinymix_set_value(struct mixer *mixer, const char *control,
                              char **values, unsigned int num_values);

unsigned int audio_test_pcm_open_cap(struct pcm_config *config, int route,
                            unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count);

void audio_test_pcm_close_cap(struct pcm *pcm);

int audio_test_pcm_read(struct pcm *pcm, char *buffer, int size);

void audio_test_pcm_open_play(struct pcm_config *config,int route,
                              unsigned int card, unsigned int device,
                              unsigned int channels, unsigned int rate,
                              enum pcm_format format,unsigned int period_size,
                              unsigned int period_count);

void audio_test_pcm_close_play(struct pcm *pcm);

void audio_test_pcm_write(struct pcm *pcm, int route, char *buffer, int size);

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_TinyALSAOpenDeviceC(JNIEnv *env, jobject obj,
                                                      jint route, jint device)
{
    enum pcm_format format = PCM_FORMAT_S16_LE;
    uint period_size = PERIOD_SIZE;
    uint period_count = PERIOD_COUNT;
    uint sample_rate = 16000;
    uint channels = 2;

    if(device != cur_dev.in.device)
    {
        if(g_pcm[route] != NULL)
        {
            LOGE("Close pcm device already opened first.\n");
            audio_test_pcm_close_cap(g_pcm[route]);
            g_pcm[route] = NULL;
        }

        char *pd;
        switch (device)
        {
            case SOUND_DEV_IN_ON_BOARD_MIC:
            case SOUND_DEV_IN_HEADPHONE_MIC:
                pd = obsd;
                break;
            case SOUND_DEV_IN_USB_MIC:
                pd = eusd;
                break;
        }
        struct mixer *mixer;
        for(int card = 0; ; card++)
        {
            mixer = mixer_open(card);
            if (!mixer) {
                LOGE("Failed to open mixer\n");
                break;
            }

            char *name = mixer->card_info.name;
            LOGI("Sound card %d mixer information: %s", card, name);
            if(strcmp(pd, name) == 0)
            {
                cur_dev.in.device = device;
                cur_dev.in.pcmC = card;
                cur_dev.in.pcmD = 0;
                if(device == SOUND_DEV_IN_ON_BOARD_MIC || device == SOUND_DEV_IN_HEADPHONE_MIC)
                {
                    char *mix_route = rn_in[device];
                    tinymix_set_value(mixer, RK809_IN_CONTROL, &mix_route, 1);
                }
                // TODO USB IN MIC or BLUETOOTH MIC
//                else
//                {}
                mixer_close(mixer);
                break;
            }
        }
    }
    else
    {
        if(g_pcm[route] != NULL)
        {
            LOGE("Already open.\n");
            return;
        }
    }

    audio_test_pcm_open_cap(g_config + route, route, cur_dev.in.pcmC, cur_dev.in.pcmD,
                            channels, sample_rate, format, period_size, period_count);
}
JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_TinyALSACloseDeviceC(JNIEnv *env, jobject thiz,
                                                             jint route)
{
    audio_test_pcm_close_cap(g_pcm[route]);
    g_pcm[route] = NULL;
    LOGI("Close finished.\n");
}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_AAudioOpenDeviceC(JNIEnv *env, jobject thiz, jint route, jint device_id)
{
    aaudio_input_stream_create(device_id);
    aaudio_input_stream_start();
}


JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_AAudioCloseDeviceC(JNIEnv *env, jobject thiz)
{
    aaudio_input_stream_close();
}

JNIEXPORT jbyteArray JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_TinyALSARead(JNIEnv *env, jobject obj,
                                                        jint route)
{
    if (g_pcm[route] == NULL)
    {
        jbyte ret_failed = -1;
        jbyteArray ret = (*env)->NewByteArray(env, 1);
        (*env)->SetByteArrayRegion(env, ret, 0, 0, &ret_failed);
        return ret;
    }

    uint size = pcm_frames_to_bytes(g_pcm[route], pcm_get_buffer_size(g_pcm[route]));
    char *buffer = malloc(size);

    int frames = audio_test_pcm_read(g_pcm[route], buffer, size);
    LOGI("Capture %d frames.", frames);
/*
    jbyteArray jframe = (*env)->NewByteArray(env, size);

    (*env)->SetByteArrayRegion(env, jframe, 0, size - 1, buffer);

    return jframe;*/
     /* 双声道数据->单声道声道 */
     jbyte * j_frame = (jbyte*)calloc(sizeof(jbyte), size / 2);
     int mono_data_index = 0;
     for(int i = 0; i < size; i += 4)
     {
         j_frame[mono_data_index] = buffer[i];
         j_frame[mono_data_index + 1] = buffer[i + 1];
         mono_data_index += 2;
     }

     jbyteArray jframe = (*env)->NewByteArray(env, size / 2);

     (*env)->SetByteArrayRegion(env, jframe, 0, size / 2 - 1, j_frame);

     free(j_frame);

    return jframe;
}

JNIEXPORT jbyteArray JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_AAudioRead(JNIEnv *env, jobject obj, jint route)
{
    char *buffer = malloc(SIZE_IN_BYTE);
    memset(buffer, 0, SIZE_IN_BYTE);

    aaudio_result_t result = aaudio_input_stream_read(route, SIZE_IN_BYTE,
                                                      buffer,10);
    LOGI("frames read = %d", result);

    // 音量扩大x倍
    short *p_short = (short *)buffer;
    int vol_amplification = 8; // 放大倍数
    for(int i = 0; i < SIZE_IN_BYTE / 2; i++)
    {
        int temp = ((int)p_short[i]) * vol_amplification;

        if(temp > 32767)
            temp = 32767;
        else if(temp < -32768)
            temp = -32768;

        p_short[i] = temp;
    }

    jbyteArray jframe = (*env)->NewByteArray(env, SIZE_IN_BYTE);
    (*env)->SetByteArrayRegion(env, jframe, 0, SIZE_IN_BYTE - 1, (jbyte *)buffer);

    free(buffer);

    return jframe;

}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_TinyALSAOpenDeviceP(JNIEnv *env, jobject thiz,
                                                           jint route, jint device)
{
    enum pcm_format format = PCM_FORMAT_S16_LE;
    unsigned int period_size = PERIOD_SIZE;
    unsigned int period_count = PERIOD_COUNT;
    unsigned int channels = 2;
    uint sample_rate = 16000;

    if(device != cur_dev.out.device)
    {
        if(g_pcm[route + 2] != NULL)
        {
            LOGE("Close pcm device already opened first.\n");
            audio_test_pcm_close_play(g_pcm[route]);
            g_pcm[route + 2] = NULL;
        }

        char *pd;
        switch (device)
        {
            case SOUND_DEV_OUT_HP:
            case SOUND_DEV_OUT_SPK:
                pd = obsd;
                break;
            case SOUND_DEV_OUT_USB_SPK:
                pd = eusd;
                break;
        }
        struct mixer *mixer;
        for(int card = 0; ; card++)
        {
            mixer = mixer_open(card);
            if (!mixer) {
                LOGE("Failed to open mixer\n");
                break;
            }

            char *name = mixer->card_info.name;
            LOGI("Sound card %d mixer information: %s", card, name);
            if(strcmp(pd, name) == 0)
            {
                cur_dev.out.device = device;
                cur_dev.out.pcmC = card;
                cur_dev.out.pcmD = 0;
                if(device == SOUND_DEV_OUT_HP || device == SOUND_DEV_OUT_SPK)
                {
                    char *mix_route = rn_out[device - 10];
                    tinymix_set_value(mixer, RK809_OUT_CONTROL, &mix_route, 1);
                }
                // TODO USB SPK or BLUETOOTH SPK
                else
                {}
                mixer_close(mixer);
                break;
            }
            mixer_close(mixer);
        }/**/
    }
    else
    {
        if(g_pcm[route + 2] != NULL)
        {
            LOGE("Already open.\n");
            return;
        }
    }

    audio_test_pcm_open_play(g_config + route + 2, route, cur_dev.out.pcmC, cur_dev.out.pcmD
                             , channels, sample_rate, format, period_size, period_count);

}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_TinyALSACloseDeviceP(JNIEnv *env, jobject thiz, jint route)
{
    audio_test_pcm_close_play(g_pcm[route + 2]);
    g_pcm[route + 2] = NULL;
}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_TinyALSAWrite(JNIEnv *env, jobject thiz, jint route,
                                                     jbyteArray data)
{
    jbyte *bytes = (*env)->GetByteArrayElements(env, data, 0);

    int size = pcm_frames_to_bytes(g_pcm[route + 2],
                                   pcm_get_buffer_size(g_pcm[route + 2]));
    char *buffer = malloc(size);
    memset(buffer, 0, size);
    memcpy(buffer, bytes, size);

    (*env)->ReleaseByteArrayElements(env, data, bytes, 0);

    audio_test_pcm_write(g_pcm[route + 2], route, buffer, size);

    free(buffer);
}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_AAudioOpenDeviceP(JNIEnv *env, jobject thiz,
                                                         jint route, jint device_id)
{
    aaudio_output_stream_create(device_id);
    aaudio_output_stream_start();
}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_AAudioCloseDeviceP(JNIEnv *env, jobject thiz)
{
    aaudio_output_stream_close();
}

JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioTrack_AAudioWrite(JNIEnv *env, jobject thiz, jint route,
                                                   jbyteArray data, jint size)
{
    jbyte *bytes;
    bytes = (*env)->GetByteArrayElements(env, data, 0);

    char *buffer = malloc(size);
    memset(buffer, 0, size);
    memcpy(buffer, bytes, size);

    aaudio_output_stream_write(route, size, buffer, 1);

    (*env)->ReleaseByteArrayElements(env, data, bytes, 0);
    free(buffer);
}

unsigned int audio_test_pcm_open_cap(struct pcm_config *config,
                                int route, unsigned int card, unsigned int device,
                                unsigned int channels, unsigned int rate,
                                enum pcm_format format, unsigned int period_size,
                                unsigned int period_count)
{
    config->channels = channels;
    config->rate = rate;
    config->period_size = period_size;
    config->period_count = period_count;
    config->format = format;
    config->start_threshold = 0;
    config->stop_threshold = 0;
    config->silence_threshold = 0;

    g_pcm[route] = pcm_open(card, device, PCM_IN, config);
    while (!g_pcm[route] || !pcm_is_ready(g_pcm[route])) {
        sleep(1);
        LOGE("Unable to open PCM device (%s)\n", pcm_get_error(g_pcm[route]));
        g_pcm[route] = pcm_open(card, device, PCM_IN, config);
    }
    return 0;
}

int audio_test_pcm_read(struct pcm *pcm, char *buffer, int size)
{
    if (pcm_read(pcm, buffer, size)) {
        LOGE("capturing error.\n");
    }
    return pcm_bytes_to_frames(pcm, size);
}

void audio_test_pcm_close_cap(struct pcm *pcm)
{
    pcm_close(pcm);
}

int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        LOGE("%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        LOGE("%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                       unsigned int rate, unsigned int bits, unsigned int period_size,
                       unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        LOGE("Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}

void audio_test_pcm_open_play(struct pcm_config *config, int route,
                              unsigned int card, unsigned int device, unsigned int channels,
                              unsigned int rate, enum pcm_format format,unsigned int period_size,
                              unsigned int period_count)
{
    config->channels = channels;
    config->rate = rate;
    config->period_size = period_size;
    config->period_count = period_count;
    config->start_threshold = 0;
    config->stop_threshold = 0;
    config->silence_threshold = 0;
    config->format = format;

    if (!sample_is_playable(card, device, channels, rate, pcm_format_to_bits(format),
                            period_size, period_count))
    {
        return;
    }

    g_pcm[route + 2] = pcm_open(card, device, PCM_OUT, config);
    while (!g_pcm[route + 2] || !pcm_is_ready(g_pcm[route + 2])) {
        LOGE("Unable to open PCM device %u (%s)\n", device, pcm_get_error(g_pcm[route + 2]));
        sleep(1);
        g_pcm[route + 2] = pcm_open(card, device, PCM_OUT, config);
    }
}

void audio_test_pcm_close_play(struct pcm *pcm)
{
    pcm_close(pcm);
}

void audio_test_pcm_write(struct pcm *pcm, int route, char *buffer, int size)
{
    LOGI("Enter audio playing, route: %d", route);

    if (pcm_write(pcm, buffer, size))
    {
        LOGE("Error playing sample\n");
    }
}


static void tinymix_set_value(struct mixer *mixer, const char *control,
                              char **values, unsigned int num_values)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    unsigned int i;

    if (isdigit(control[0]))
        ctl = mixer_get_ctl(mixer, atoi(control));
    else
        ctl = mixer_get_ctl_by_name(mixer, control);

    if (!ctl) {
        LOGE("Invalid mixer control\n");
        return;
    }

    type = mixer_ctl_get_type(ctl);
    num_ctl_values = mixer_ctl_get_num_values(ctl);

    if (isdigit(values[0][0]) || (values[0][0]=='-' && isdigit(values[0][1])))
    {
        if (num_values == 1)
        {
            /* Set all values the same */
            int value = atoi(values[0]);

            for (i = 0; i < num_ctl_values; i++) {
                if (mixer_ctl_set_value(ctl, i, value)) {
                    LOGE("Error: invalid value\n");
                    return;
                }
            }
        }
        else
        {
            /* Set multiple values */
            if (num_values > num_ctl_values) {
                LOGE("Error: %d values given, but control only takes %d\n",
                        num_values, num_ctl_values);
                return;
            }
            for (i = 0; i < num_values; i++) {
                if (mixer_ctl_set_value(ctl, i, atoi(values[i]))) {
                    LOGE("Error: invalid value for index %d\n", i);
                    return;
                }
            }
        }
    }
    else
    {
        if (type == MIXER_CTL_TYPE_ENUM)
        {
            if (num_values != 1)
            {
                LOGE("Enclose strings in quotes and try again\n");
                return;
            }
            if (mixer_ctl_set_enum_by_string(ctl, values[0]))
                LOGE("Error: invalid enum value\n");
        }
        else
        {
            LOGE("Error: only enum types can be set with strings\n");
        }
    }
}

/*-----------------------------------------------------------------------------------------------
struct pcm *open_device_c( unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count)
{
    struct pcm_config config;
    struct pcm *pcm;

    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        LOGE("Unable to open PCM device (%s)\n", pcm_get_error(pcm));
        return NULL;
    }
    return pcm;
}


JNIEXPORT jint JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_open(JNIEnv *env, jobject thiz, jint phone_side,
                                             jint device)
{
    if(phone_side == PHONE_SIDE_OUR)
    {
        if(our_side.is_open == 0)
        {
            if(our_side.card == -1)
            {
                int card = 0;
                struct mixer *mixer;
                for(;; card++)
                {
                    mixer = mixer_open(card);

                    if (!mixer)
                    {
                        LOGE("Failed to open mixer\n");
                        LOGI("No Mic specified found.\n");

//                        jbyte ret_failed = -1;
//                        jbyteArray ret = (*env)->NewByteArray(env, 1);
//                        (*env)->SetByteArrayRegion(env, ret, 0, 0, &ret_failed);

                        return -1;
                    }

                    char *name = mixer->card_info.name;
                    LOGI("Sound card %d mixer information: %s", card, name);

                    if(device == SOUND_DEV_REC_ON_BOARD_MIC)
                    {
                        // Sound card rk809 detected.
                        if (strcmp(obsd, name) == 0)
                        {
                            char *mix_route = "Main Mic";
                            tinymix_set_value(mixer, "1", &mix_route, 1);
                            our_side.card = card;
                            our_side.sub_dev = 0;
                            mixer_close(mixer);
                            break;
                        }
                    }
                    // TODO other devices
                    mixer_close(mixer);
                }
            }

            uint channels = 2;
            uint rate = 16000;
            uint period_size = PERIOD_SIZE;
            uint period_count = PERIOD_COUNT;
            enum pcm_format format = PCM_FORMAT_S16_LE;

            our_side.ppcm = open_device_c( our_side.card, our_side.sub_dev, channels,
                                     rate, format, period_size, period_count);
            // open successfully
            if(our_side.ppcm != NULL)
            {
                our_side.is_open = 1;
                our_side.st = PCM_STREAM_TYPE_IN;
                return 0;
            }
            return -1;
        }
        else
            return 0;
    }

    // TODO PHONE_SIDE_OPPOSITE
}


JNIEXPORT void JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_close(JNIEnv *env, jobject thiz, jint phone_side)
{
    if(phone_side == PHONE_SIDE_OUR)
    {
        if(our_side.is_open == 1)
        {
            pcm_close(our_side.ppcm);
            our_side.ppcm = NULL;
            our_side.is_open = 0;
        }
    }
    // TODO
    else if(phone_side == PHONE_SIDE_OPPOSITE)
    {}
}

JNIEXPORT jbyteArray JNICALL
Java_com_ndk_audiotestapp_MyAudioRecord_read(JNIEnv *env, jobject thiz, jint phone_side,
                                             jint sample_rate)
{
    if(phone_side == PHONE_SIDE_OUR)
    {
        if(our_side.is_open == 1)
        {
            //unsigned int bytes_read = 0;
            uint channels = 2;
            uint rate = 16000;
            uint period_size = PERIOD_SIZE;
            uint period_count = PERIOD_COUNT;
            enum pcm_format format = PCM_FORMAT_S16_LE;

            printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
                   pcm_format_to_bits(format));
            unsigned int size;
            char *buffer;
            size = (period_size * period_count) * (pcm_format_to_bits(format) >> 3) * channels;
            buffer = malloc(size);

            if (!buffer)
            {
                LOGE("Unable to allocate %d bytes\n", size);
                free(buffer);

                jbyte ret_failed = -1;
                jbyteArray ret = (*env)->NewByteArray(env, 1);
                (*env)->SetByteArrayRegion(env, ret, 0, 0, &ret_failed);

                return ret;
            }

            if (!pcm_read(our_side.ppcm, buffer, size))
            {
                LOGI("TinyALSA Capture : %d frames.\n", period_size * period_count);
                //bytes_read += size;
                jbyteArray ret = (*env)->NewByteArray(env, size);
                (*env)->SetByteArrayRegion(env, ret, 0, size - 1, (jbyte *)buffer);
                free(buffer);
                return ret;
            }
            else
            {
                LOGI("TinyALSA Capture Failed.\n");
                free(buffer);
                jbyte ret_failed = -1;
                jbyteArray ret = (*env)->NewByteArray(env, 1);
                (*env)->SetByteArrayRegion(env, ret, 0, 0, &ret_failed);
                return ret;
            }


        }
        else
        {
            LOGE("Device isn't opened.\n");
            jbyte ret_failed = -1;
            jbyteArray ret = (*env)->NewByteArray(env, 1);
            (*env)->SetByteArrayRegion(env, ret, 0, 0, &ret_failed);
        }
    }
    // TODO
    else
    {}
}
 ---------------------------------------------------------------------------------------------*/