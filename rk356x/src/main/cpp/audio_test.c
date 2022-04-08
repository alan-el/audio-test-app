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
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define SOUND_DEV_IN_NONE           -1
#define SOUND_DEV_IN_ON_BOARD_MIC   0
#define SOUND_DEV_IN_HEADPHONE_MIC  1
#define SOUND_DEV_IN_USB_MIC        2
#define SOUND_DEV_IN_EX_USB_MIC     3

#define SOUND_DEV_OUT_NONE          -1
#define SOUND_DEV_OUT_HP            10
#define SOUND_DEV_OUT_SPK           11
#define SOUND_DEV_OUT_USB_SPK       12
#define SOUND_DEV_OUT_EX_USB_SPK    13

#define RK809_IN_CONTROL        "1"
#define RK809_OUT_CONTROL       "0"

// On board sound device name
static const char *obsd = "rockchip,rk809-codec\0";
// USB sound device name
static const char *usd = "USB Audio Device\0";
// extend USB sound device name
static const char *eusd = "USB PnP Sound Device\0";

// mixer route name
const char *rn_in[] =
{
    "Main Mic",
    "Hands Free Mic"
};

const char *rn_out[] =
{
    "HP",
    "SPK"
};

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

/** maintain current audio device used on our side and opposite side
 *  cur_dev[0]: our side audio device
 *  cur_dev[1]: opposite side audio device
 */
cur_devices_t cur_dev[2] =
{
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
    },
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
    }
};


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

    if(device != cur_dev[route].in.device)
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
                pd = (char *)obsd;
                break;
            case SOUND_DEV_IN_USB_MIC:
                pd = (char *)usd;
                break;
            case SOUND_DEV_IN_EX_USB_MIC:
                pd = (char *)eusd;
                break;
            default:
                LOGE("Invalid device.\n");
                return;
        }
        struct mixer *mixer;
        for(int card = 0; ; card++)
        {
            mixer = mixer_open(card);
            if (!mixer) {
                LOGE("Failed to open mixer\n");
                break;
            }

            char *name = (char *)mixer->card_info.name;
            LOGI("Sound card %d mixer information: %s", card, name);
            if(strcmp(pd, name) == 0)
            {
                cur_dev[route].in.device = device;
                cur_dev[route].in.pcmC = card;
                cur_dev[route].in.pcmD = 0;

                if(device == SOUND_DEV_IN_ON_BOARD_MIC || device == SOUND_DEV_IN_HEADPHONE_MIC)
                {
                    char *mix_route = (char *)rn_in[device];
                    tinymix_set_value(mixer, RK809_IN_CONTROL, &mix_route, 1);
                }

                else if(device == SOUND_DEV_IN_USB_MIC || device == SOUND_DEV_IN_EX_USB_MIC)
                {
                    sample_rate = 48000;
                }

                if(device == SOUND_DEV_IN_USB_MIC)
                {
                    channels = 1;
                }
                mixer_close(mixer);
                break;
            }
            mixer_close(mixer);
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

    audio_test_pcm_open_cap(g_config + route, route, cur_dev[route].in.pcmC, cur_dev[route].in.pcmD,
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

    int size = pcm_frames_to_bytes(g_pcm[route], pcm_get_buffer_size(g_pcm[route]));
    int change_x = 2;
    char *buffer = malloc(size);

    int frames = audio_test_pcm_read(g_pcm[route], buffer, size);
    LOGI("Capture %d frames. route = %d", frames, route);

    /* 双声道数据->单声道声道 */
    if(g_config[route].channels == 2)
    {
        size /= 2;
        change_x = 4;
    }


    jbyte * j_frame = (jbyte*)calloc(sizeof(jbyte), size);

    int mono_data_index = 0;
    for(int i = 0; i < size; i += change_x)
    {
         j_frame[mono_data_index] = buffer[i];
         j_frame[mono_data_index + 1] = buffer[i + 1];
         mono_data_index += 2;
    }

    free(buffer);

    /* down sample rate: 48kHz -> 16kHz */
    if(g_config[route].rate == 48000)
    {
        size /= 3;
        jbyte * ds_frame = (jbyte*)calloc(sizeof(jbyte), size);
        for(int i = 0; i < size; i += 2)
        {
            *(ds_frame + i) = *(j_frame + (3 * i));
            *(ds_frame + i + 1) = *(j_frame + (3 * i) + 1);
        }

        jbyteArray jframe = (*env)->NewByteArray(env, size);

        (*env)->SetByteArrayRegion(env, jframe, 0, size - 1, ds_frame);

        free(j_frame);
        free(ds_frame);

        return jframe;
    }
    else
    {
        jbyteArray jframe = (*env)->NewByteArray(env, size);

        (*env)->SetByteArrayRegion(env, jframe, 0, size - 1, j_frame);

        free(j_frame);

        return jframe;
    }
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

    if(device != cur_dev[route].out.device)
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
                pd = (char *)obsd;
                break;
            case SOUND_DEV_OUT_USB_SPK:
                pd = (char *)eusd;
                break;
            case SOUND_DEV_OUT_EX_USB_SPK:
                pd = (char *)eusd;
                break;
            default:
                LOGE("Invalid device.\n");
                return;
        }
        struct mixer *mixer;
        for(int card = 0; ; card++)
        {
            mixer = mixer_open(card);
            if (!mixer) {
                LOGE("Failed to open mixer\n");
                break;
            }

            char *name = (char *)mixer->card_info.name;
            LOGI("Sound card %d mixer information: %s", card, name);
            if(strcmp(pd, name) == 0)
            {
                cur_dev[route].out.device = device;
                cur_dev[route].out.pcmC = card;
                cur_dev[route].out.pcmD = 0;
                if(device == SOUND_DEV_OUT_HP || device == SOUND_DEV_OUT_SPK)
                {
                    char *mix_route = (char *)rn_out[device - 10];
                    tinymix_set_value(mixer, RK809_OUT_CONTROL, &mix_route, 1);
                }

                else if(device == SOUND_DEV_OUT_USB_SPK || device == SOUND_DEV_OUT_EX_USB_SPK)
                {
                    sample_rate = 48000;
                }
                mixer_close(mixer);
                break;
            }
            mixer_close(mixer);
        }
    }
    else
    {
        if(g_pcm[route + 2] != NULL)
        {
            LOGE("Already open.\n");
            return;
        }
    }

    audio_test_pcm_open_play(g_config + route + 2, route, cur_dev[route].out.pcmC, cur_dev[route].out.pcmD
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
    if(g_pcm[route + 2] == NULL)
    {
        LOGE("pcm device is NULL.\n");
        return;
    }

    jbyte *bytes = (*env)->GetByteArrayElements(env, data, 0);

    int size = pcm_frames_to_bytes(g_pcm[route + 2],
                                   pcm_get_buffer_size(g_pcm[route + 2]));
    /* bookmark TODO if usb device, 16kHz -> 48kHz */
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

