//
// Created by Alan on 2021/11/25.
//

#ifndef AUDIO_TEST_APP_NDK_AAUDIO_H
#define AUDIO_TEST_APP_NDK_AAUDIO_H

#include <aaudio/AAudio.h>
/*---------------------------------TingALSA-------------------------------------*/
#define PERIOD_SIZE (512)
#define PERIOD_COUNT (2)

/*---------------------------------AAudio-------------------------------------*/
#define USB_MIC_SAMPLE_RATE (16000)
#define USB_MIC_CHANNLES    (1)
#define USB_SPK_SAMPLE_RATE (16000)
#define USB_SPK_CHANNLES    (1)
#define SIZE_IN_BYTE        (PERIOD_SIZE * PERIOD_COUNT * 2)

void aaudio_input_stream_create(int device_id);
void aaudio_input_stream_close(void);
void aaudio_input_stream_start(void);
aaudio_result_t aaudio_input_stream_read(int route, int32_t numFrames, char *buffer,
                                         int64_t timeoutNanoseconds);

void aaudio_output_stream_create(int device_id);
void aaudio_output_stream_close(void);
void aaudio_output_stream_start(void);
aaudio_result_t aaudio_output_stream_write(int route, int32_t size, char *buffer,
                                           int64_t timeoutNanoseconds);
#endif //AUDIO_TEST_APP_NDK_AAUDIO_H
