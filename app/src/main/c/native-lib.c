#include <aaudio/AAudio.h>
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <media/NdkMediaExtractor.h>
#include <malloc.h>
#include <string.h>
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <tgmath.h>

#define APP_NAME "noice"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, APP_NAME, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, APP_NAME, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, APP_NAME, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, APP_NAME, __VA_ARGS__))

static const int config_channel_count = 1;
static const int config_sample_rate = 44100;

typedef struct {
    _Atomic (int16_t *) data;
    size_t size;
} Data;

typedef struct {
    Data quick;
    Data data;
    _Atomic float volume;
    _Atomic bool is_playing;
    size_t cursor;
} Buffer;

_Atomic bool isStopped = false;

static float curve(float volume) {
    return (1.f / 32768.f) * (exp2(volume) - 1.f);
}

static bool
buffer_initialise(AAssetManager *const assetManager, const char *filename, Data *const output) {
    LOGD(__func__);
    AAsset *asset = NULL;
    AMediaFormat *format = NULL;
    AMediaExtractor *extractor = NULL;
    AMediaCodec *codec = NULL;
    void *output_data = NULL;
    media_status_t result = AMEDIA_ERROR_UNKNOWN;

    // Asset Manager
    {
        asset = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
        if (!asset) {
            LOGE("Error AAssetManager_open, Failed to open asset %s", filename);
            goto error;
        }
    }

    // Media Extractor
    {
        off_t start, length;
        int fd = AAsset_openFileDescriptor(asset, &start, &length);
        extractor = AMediaExtractor_new();
        result = AMediaExtractor_setDataSourceFd(extractor, fd, (off64_t) start, (off64_t) length);
        if (result != AMEDIA_OK) {
            LOGE("Error AMediaExtractor_setDataSourceFd, err %d", result);
            goto error;
        }
    }

    // Format
    {
        format = AMediaExtractor_getTrackFormat(extractor, 0);
        int32_t sample_rate;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sample_rate)) {
            LOGD("Source sample rate %d", sample_rate);
            if (sample_rate != config_sample_rate) {
                LOGE("Input (%d) and output (%d) sample rates do not match. NDK decoder does not support resampling.",
                     sample_rate, config_sample_rate);
                goto error;
            }
        } else {
            LOGE("Failed to get sample rate");
            goto error;
        }
        int32_t channel_count;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channel_count)) {
            LOGD("Got channel count %d", channel_count);
            if (channel_count != config_channel_count) {
                LOGE("NDK decoder does not support different input (%d) and output (%d) channel counts",
                     channel_count, config_channel_count);
                goto error;
            }
        } else {
            LOGE("Failed to get channel count");
            goto error;
        }
        LOGD("Output format %s", AMediaFormat_toString(format));
    }

    // Codec
    {
        const char *mime_type;
        if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime_type)) {
            LOGD("Got mime type %s", mime_type);
        } else {
            LOGE("Error AMediaFormat_getString");
            goto error;
        }
        AMediaExtractor_selectTrack(extractor, 0);
        codec = AMediaCodec_createDecoderByType(mime_type);
        AMediaCodec_configure(codec, format, 0, 0, 0);
        AMediaCodec_start(codec);
    }

    // DECODE
    {
        size_t output_alloc_size = AAsset_getLength(asset);
        size_t output_cursor = 0;
        output_data = malloc(output_alloc_size);
        bool is_extracting = true;
        bool is_decoding = true;
        while (is_extracting || is_decoding) {
            if(isStopped) {
                goto error;
            }
            if (is_extracting) {
                ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(codec, 2000);
                if (inputIndex < 0) {
                    if (inputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                        LOGW("AMediaCodec_dequeueInputBuffer: try again later");
                    } else {
                        LOGE("Error AMediaCodec_dequeueInputBuffer, err %d", (int) inputIndex);
                        goto error;
                    }
                } else {
                    size_t inputSize;
                    uint8_t *inputBuffer = AMediaCodec_getInputBuffer(codec, inputIndex,
                                                                      &inputSize);
                    ssize_t sampleSize = AMediaExtractor_readSampleData(extractor, inputBuffer,
                                                                        inputSize);
                    int64_t presentationTimeUs = AMediaExtractor_getSampleTime(extractor);
                    if (sampleSize > 0) {
                        AMediaCodec_queueInputBuffer(codec, inputIndex, 0, sampleSize,
                                                     presentationTimeUs, 0);
                        AMediaExtractor_advance(extractor);
                    } else {
                        LOGD("End of extractor data stream");
                        is_extracting = false;
                        AMediaCodec_queueInputBuffer(codec, inputIndex, 0, 0, presentationTimeUs,
                                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    }
                }
            }
            if (is_decoding) {
                AMediaCodecBufferInfo info;
                ssize_t index = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
                if (index >= 0) {
                    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                        LOGD("Reached end of decoding stream");
                        is_decoding = false;
                    }
                    size_t dummy;
                    uint8_t *output_buffer = AMediaCodec_getOutputBuffer(codec, index, &dummy);
                    size_t required_output_alloc_size = output_cursor + info.size;
                    while (required_output_alloc_size > output_alloc_size) {
                        output_alloc_size *= 2;
                        output_data = realloc(output_data, output_alloc_size);
                    }
                    if (output_data) {
                        memcpy((uint8_t *) output_data + output_cursor, output_buffer, info.size);
                        output_cursor = required_output_alloc_size;
                    }
                    result = AMediaCodec_releaseOutputBuffer(codec, index, false);
                    if (result != AMEDIA_OK) {
                        LOGE("Error AMediaCodec_releaseOutputBuffer, err %d", result);
                        goto error;
                    }
                    if (output_data == NULL) {
                        LOGE("Error realloc");
                        goto error;
                    }
                } else {
                    switch (index) {
                        case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
                            LOGW("AMediaCodec_dequeueOutputBuffer: try again later");
                            break;
                        case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                            LOGW("AMediaCodec_dequeueOutputBuffer: output buffers changed");
                            break;
                        case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
                            LOGW("AMediaCodec_dequeueOutputBuffer: output outputFormat changed");
                            format = AMediaCodec_getOutputFormat(codec);
                            LOGD("outputFormat changed to: %s", AMediaFormat_toString(format));
                            break;
                    }
                }
            }
        }
        output->size = output_cursor / sizeof(int16_t);
        output->data = (int16_t *) output_data;
        output_data = NULL;
        result = AMEDIA_OK;
    }
    error:
    AMediaFormat_delete(format);
    AMediaCodec_delete(codec);
    AMediaExtractor_delete(extractor);
    AAsset_close(asset);
    free(output_data);
    return AMEDIA_OK != result;
}

static aaudio_data_callback_result_t
audio_callback(AAudioStream *const audio_stream, void *const user_data, void *const audio_data,
               int32_t const num_frames) {
    (void) audio_stream;
    float *floatData = (float *) audio_data;
    Buffer *buffers = (Buffer *) user_data;
    for (int i = 0; i < num_frames; ++i) {
        for (int j = 0; j < config_channel_count; j++) {
            float sample = .0f;
            for (int k = 0; k < 8; ++k) {
                Buffer *buffer = &buffers[k];
                if (buffer->is_playing) {
                    if (buffer->data.data) {
                        sample += buffer->data.data[buffer->cursor] * buffer->volume;
                        if (++buffer->cursor >= buffer->data.size) {
                            buffer->cursor = 0;
                        }
                    } else if (buffer->quick.data) {
                        sample += buffer->quick.data[buffer->cursor] * buffer->volume;
                        if (++buffer->cursor >= buffer->quick.size) {
                            buffer->cursor = 0;
                        }
                    }
                }
            }
            floatData[i * config_channel_count + j] = sample;
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static AAudioStream *stream = NULL;
static Buffer buffers[8] = {};

static void audio_stream_open() {
    LOGD(__func__);
    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAUDIO_OK;
    result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudio_createStreamBuilder, err %d", result);
        goto error;
    }
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setSampleRate(builder, config_sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, config_channel_count);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, audio_callback, buffers);
    result = AAudioStreamBuilder_openStream(builder, &stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStreamBuilder_openStream, err %d", result);
        goto error;
    }
    error:
    {
        aaudio_result_t delete_result = AAudioStreamBuilder_delete(builder);
        if (delete_result != AAUDIO_OK) {
            LOGE("Error AAudioStreamBuilder_delete, err %d", delete_result);
        }
    }
}

static void buffer_close(Buffer *const buffer) {
    LOGD(__func__);
    free(buffer->data.data);
    buffer->data.size = 0;
    free(buffer->quick.data);
    buffer->quick.size = 0;
    buffer->volume = curve(.5f);
    buffer->cursor = 0;
    buffer->is_playing = false;
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_initialise(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) class;
    audio_stream_open();
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_destroy(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    aaudio_result_t result = AAudioStream_close(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_close, err %d", result);
    }
    stream = NULL;
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_setNativeChannelVolume(JNIEnv *env, jclass class,
                                                                   jint i, jfloat volume) {
    LOGD(__func__);
    (void) env;
    (void) class;
    buffers[i].volume = curve(volume);
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_setNativeChannelPlaying(JNIEnv *env, jclass class,
                                                                    jint i, jboolean is_playing) {
    LOGD(__func__);
    (void) env;
    (void) class;
    buffers[i].is_playing = is_playing;
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_start(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    aaudio_result_t result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_requestStart, err %d", result);
    }
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_AudioService_stop(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    aaudio_result_t result = AAudioStream_requestStop(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_requestStop, err %d", result);
    }
}


JNIEXPORT void JNICALL
Java_uk_golbourn_noice_MainActivity_quick_1initialise(JNIEnv *env, jclass class, jobjectArray files,
                                                      jobject jAssetManager) {
    LOGD(__func__);
    (void) class;
    AAssetManager *assetManager = AAssetManager_fromJava(env, jAssetManager);
    if (assetManager == NULL) {
        LOGE("Could not obtain the AAssetManager");
        return;
    }
    for (int i = 0; i < 8; ++i) {
        if(!isStopped) {
            Buffer *buffer = &buffers[i];
            buffer->volume = curve(.5f);
            buffer->is_playing = false;
            buffer->cursor = 0;
            jstring file = (jstring) (*env)->GetObjectArrayElement(env, files, i);
            const char *filename = (*env)->GetStringUTFChars(env, file, 0);
            bool error = buffer_initialise(assetManager, filename, &buffer->quick);
            if (error) {
                LOGE("Could not load %s", filename);
            }
            (*env)->ReleaseStringUTFChars(env, file, filename);
            if (error) {
                return;
            }
        }
    }
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_MainActivity_lazy_1initialise(JNIEnv *env, jclass class, jobjectArray files,
                                                     jobject jAssetManager) {
    LOGD(__func__);
    (void) class;
    AAssetManager *assetManager = AAssetManager_fromJava(env, jAssetManager);
    if (assetManager == NULL) {
        LOGE("Could not obtain the AAssetManager");
        return;
    }
    for (int i = 0; i < 8; ++i) {
        if(!isStopped) {
            Buffer *buffer = &buffers[i];
            jstring file = (jstring) (*env)->GetObjectArrayElement(env, files, i);
            const char *filename = (*env)->GetStringUTFChars(env, file, 0);
            bool error = buffer_initialise(assetManager, filename, &buffer->data);
            if (error) {
                LOGE("Could not load %s", filename);
            }
            (*env)->ReleaseStringUTFChars(env, file, filename);
            if (error) {
                return;
            }
        }
    }
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_MainActivity_stop(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    isStopped = true;
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_MainActivity_start(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    isStopped = false;
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_MainActivity_destroy(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    for (int k = 0; k < 8; ++k) {
        buffer_close(&buffers[k]);
    }
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_NotificationActionReceiver_start(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    aaudio_result_t result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_requestStart, err %d", result);
    }
}

JNIEXPORT void JNICALL
Java_uk_golbourn_noice_NotificationActionReceiver_stop(JNIEnv *env, jclass class) {
    LOGD(__func__);
    (void) env;
    (void) class;
    aaudio_result_t result = AAudioStream_requestStop(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_requestStop, err %d", result);
    }
}
