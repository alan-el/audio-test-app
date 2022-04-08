//
// Created by Alan on 2021/11/25.
//

#include "ndk_aaudio.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <android/log.h>
#include <aaudio/AAudio.h>

#define LOG_TAG "mAAudio"
#define LOGI_A(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static AAudioStream *input_stream = NULL;
static AAudioStream *output_stream = NULL;

void aaudio_input_stream_create(int device_id) {
    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGI_A("Create Failed, reason: %s", AAudio_convertResultToText(result));
    }

    //AAudioStreamBuilder_setDataCallback(builder,mAAudioStreamDataCallbackRecord, NULL);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDeviceId(builder, device_id);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_SPEECH);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, USB_MIC_SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(builder, USB_MIC_CHANNLES);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    //AAudioStreamBuilder_setBufferCapacityInFrames(builder, 4096);

    result = AAudioStreamBuilder_openStream(builder, &input_stream);
    if (result != AAUDIO_OK) {
        LOGI_A("Open Failed, reason: %s", AAudio_convertResultToText(result));
    }
    AAudioStreamBuilder_delete(builder);

    int32_t device_id_set = AAudioStream_getDeviceId(input_stream);
    LOGI_A("Device ID = %d\n", device_id_set);
    aaudio_direction_t audion_direction_set = AAudioStream_getDirection(input_stream);
    LOGI_A("Audio Direction = %d\n", audion_direction_set);
    aaudio_sharing_mode_t sharing_mode = AAudioStream_getSharingMode(input_stream);
    LOGI_A("Sharing Mode = %d\n", sharing_mode);
    int32_t sample_rate_set = AAudioStream_getSampleRate(input_stream);
    LOGI_A("Sample Rate = %d\n", sample_rate_set);
    int32_t channel_count = AAudioStream_getChannelCount(input_stream);
    LOGI_A("Channel Count = %d\n", channel_count);
    aaudio_format_t format = AAudioStream_getFormat(input_stream);
    LOGI_A("Format = %d\n", format);
    int32_t buffer_cap = AAudioStream_getBufferCapacityInFrames(input_stream);
    LOGI_A("Buffer Capacity = %d\n", buffer_cap);
}

void aaudio_input_stream_close(void)
{
    AAudioStream_close(input_stream);
    input_stream = NULL;
}

void aaudio_input_stream_start(void)
{
    aaudio_result_t result = AAudioStream_requestStart(input_stream);

    if(result != AAUDIO_OK)
    {
        LOGI_A("Start Failed, reason: %s", AAudio_convertResultToText(result));
    }
}

aaudio_result_t aaudio_input_stream_read(int route, int32_t size, char *buffer,
                                         int64_t timeoutNanoseconds)
{
    int numFrames = size / 2 / AAudioStream_getChannelCount(input_stream);
    aaudio_result_t result =  AAudioStream_read(input_stream, buffer, numFrames,
                                                timeoutNanoseconds);
    if(result != numFrames)
    {
        LOGI_A("read Frames not equals required. Frames = %d", result);
    }
    return  result;
}

void aaudio_output_stream_create(int device_id)
{
    if(output_stream != NULL)
    {
        aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
        aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
        int64_t timeoutNanos = 1000 * 1000;
        AAudioStream_requestStop(output_stream);
        AAudioStream_waitForStateChange(output_stream, inputState, &nextState, timeoutNanos);
        AAudioStream_close(output_stream);
        output_stream = NULL;
    }
    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if(result != AAUDIO_OK)
    {
        LOGI_A("Create Failed, reason: %s", AAudio_convertResultToText(result));
    }


    //AAudioStreamBuilder_setDataCallback(builder,mAAudioStreamDataCallbackPlay, NULL);
    //AAUDIO_PERFORMANCE_MODE_LOW_LATENCY是低延迟方式。
    AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

    AAudioStreamBuilder_setDeviceId(builder, device_id);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_SPEECH);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, USB_SPK_SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(builder, USB_SPK_CHANNLES);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 1024 * 4);

    result = AAudioStreamBuilder_openStream(builder, &output_stream);
    if(result != AAUDIO_OK)
    {
        LOGI_A("Open Failed, reason: %s", AAudio_convertResultToText(result));
    }
    AAudioStreamBuilder_delete(builder);

    int32_t device_id_set =  AAudioStream_getDeviceId(output_stream);
    LOGI_A("Device ID = %d\n", device_id_set);
    aaudio_direction_t audion_direction_set =  AAudioStream_getDirection(output_stream);
    LOGI_A("Audio Direction = %d\n", audion_direction_set);
    aaudio_sharing_mode_t sharing_mode =  AAudioStream_getSharingMode(output_stream);
    LOGI_A("Sharing Mode = %d\n", sharing_mode);
    int32_t sample_rate =  AAudioStream_getSampleRate(output_stream);
    LOGI_A("Sample Rate = %d\n", sample_rate);
    int32_t channel_count =  AAudioStream_getChannelCount(output_stream);
    LOGI_A("Channel Count = %d\n", channel_count);
    aaudio_format_t format =  AAudioStream_getFormat(output_stream);
    LOGI_A("Format = %d\n", format);
    int32_t buffer_cap =  AAudioStream_getBufferCapacityInFrames(output_stream);
    LOGI_A("Buffer Capacity = %d\n", buffer_cap);

}
void aaudio_output_stream_close(void)
{
    AAudioStream_close(output_stream);
    output_stream = NULL;
}

void aaudio_output_stream_start(void)
{
    aaudio_result_t result = AAudioStream_requestStart(output_stream);

    if(result != AAUDIO_OK)
    {
        LOGI_A("Start Failed, reason: %s", AAudio_convertResultToText(result));
    }
}


aaudio_result_t aaudio_output_stream_write(int route, int32_t size, char *buffer,
                                           int64_t timeoutNanoseconds)
{
    int numFrames = size / 2 / AAudioStream_getChannelCount(output_stream);
    aaudio_result_t result =  AAudioStream_write(output_stream, buffer, numFrames,
                                                 timeoutNanoseconds);
    if(result != numFrames)
    {
        // TODO 
        LOGI_A("write Frames not equals required. Frames = %d", result);
    }
    return  result;
}


