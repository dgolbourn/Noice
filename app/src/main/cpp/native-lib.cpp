// Write C++ code here.
//
// Do not forget to dynamically load the C++ library into your application.
//
// For instance,
//
// In MainActivity.java:
//    static {
//       System.loadLibrary("noice");
//    }
//
// Or, in MainActivity.kt:
//    companion object {
//      init {
//         System.loadLibrary("noice")
//      }
//    }
#include <aaudio/AAudio.h>
#include <cstdint>
#include <cinttypes>
#include <sys/types.h>
#include <cstring>
#include <android/asset_manager.h>
#include <android/log.h>
#include <media/NdkMediaExtractor.h>
#include <malloc.h>

#define APP_NAME "noice"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, APP_NAME, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, APP_NAME, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, APP_NAME, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, APP_NAME, __VA_ARGS__))


constexpr int kMaxCompressionRatio = 12;
constexpr int kChannelCount = 2;
constexpr int kSampleRate = 48000;

struct AudioProperties {
    size_t channelCount;
    size_t sampleRate;
};

struct AAssetDataSource {
    int16_t* buffer;
    size_t size;
};

AAudioStream *stream = NULL;

static int decode(AAssetManager *assetManager, const char *filename, AudioProperties targetProperties, AAssetDataSource **output) {
    *output = NULL;
    AAsset *asset = NULL;
    AMediaFormat *format = NULL;
    AMediaExtractor *extractor = NULL;
    AMediaCodec *codec = NULL;

    // Asset Manager
    {
        asset = AAssetManager_open(assetManager, filename, AASSET_MODE_UNKNOWN);
        if (!asset) {
            LOGE("Failed to open asset %s", filename);
            goto error;
        }
    }

    // Media Extractor
    {
        off_t start, length;
        int fd = AAsset_openFileDescriptor(asset, &start, &length);
        extractor = AMediaExtractor_new();
        media_status_t amresult = AMediaExtractor_setDataSourceFd(extractor, fd,
                                                                  static_cast<off64_t>(start),
                                                                  static_cast<off64_t>(length));
        if (amresult != AMEDIA_OK) {
            LOGE("Error setting extractor data source, err %d", amresult);
            goto error;
        }
    }

    // Format
    {
        format = AMediaExtractor_getTrackFormat(extractor, 0);

        int32_t sampleRate;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate)) {
            LOGD("Source sample rate %d", sampleRate);
            if (sampleRate != targetProperties.sampleRate) {
                LOGE("Input (%d) and output (%d) sample rates do not match. "
                     "NDK decoder does not support resampling.",
                     sampleRate,
                     targetProperties.sampleRate);
                goto error;
            }
        } else {
            LOGE("Failed to get sample rate");
            goto error;
        }

        int32_t channelCount;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount)) {
            LOGD("Got channel count %d", channelCount);
            if (channelCount != targetProperties.channelCount) {
                LOGE("NDK decoder does not support different "
                     "input (%d) and output (%d) channel counts",
                     channelCount,
                     targetProperties.channelCount);
            }
        } else {
            LOGE("Failed to get channel count");
            goto error;
        }

        LOGD("Output format %s", AMediaFormat_toString(format));
    }

    // Codec
    {
        const char *mimeType;
        if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mimeType)) {
            LOGD("Got mime type %s", mimeType);
        } else {
            LOGE("Failed to get mime type");
            goto error;
        }

        AMediaExtractor_selectTrack(extractor, 0);
        codec = AMediaCodec_createDecoderByType(mimeType);
        AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
        AMediaCodec_start(codec);
    }

    // DECODE
    {
        off_t assetSize = AAsset_getLength(asset);
        const long maximumDataSizeInBytes = kMaxCompressionRatio * assetSize * sizeof(int16_t);
        uint8_t *decodedData = (uint8_t *)malloc(maximumDataSizeInBytes);
        size_t bytesWritten = 0;

        bool isExtracting = true;
        bool isDecoding = true;
        while (isExtracting || isDecoding) {

            if (isExtracting) {

                // Obtain the index of the next available input buffer
                ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(codec, 2000);
                //LOGV("Got input buffer %d", inputIndex);

                // The input index acts as a status if its negative
                if (inputIndex < 0) {
                    if (inputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                        // LOGV("Codec.dequeueInputBuffer try again later");
                    } else {
                        LOGE("Codec.dequeueInputBuffer unknown error status");
                    }
                } else {

                    // Obtain the actual buffer and read the encoded data into it
                    size_t inputSize;
                    uint8_t *inputBuffer = AMediaCodec_getInputBuffer(codec, inputIndex,
                                                                      &inputSize);
                    //LOGV("Sample size is: %d", inputSize);

                    ssize_t sampleSize = AMediaExtractor_readSampleData(extractor, inputBuffer,
                                                                        inputSize);
                    auto presentationTimeUs = AMediaExtractor_getSampleTime(extractor);

                    if (sampleSize > 0) {

                        // Enqueue the encoded data
                        AMediaCodec_queueInputBuffer(codec, inputIndex, 0, sampleSize,
                                                     presentationTimeUs,
                                                     0);
                        AMediaExtractor_advance(extractor);

                    } else {
                        LOGD("End of extractor data stream");
                        isExtracting = false;

                        // We need to tell the codec that we've reached the end of the stream
                        AMediaCodec_queueInputBuffer(codec, inputIndex, 0, 0,
                                                     presentationTimeUs,
                                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    }
                }
            }

            if (isDecoding) {
                // Dequeue the decoded data
                AMediaCodecBufferInfo info;
                ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);

                if (outputIndex >= 0) {

                    // Check whether this is set earlier
                    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                        LOGD("Reached end of decoding stream");
                        isDecoding = false;
                    }

                    // Valid index, acquire buffer
                    size_t outputSize;
                    uint8_t *outputBuffer = AMediaCodec_getOutputBuffer(codec, outputIndex,
                                                                        &outputSize);

                    /*LOGV("Got output buffer index %d, buffer size: %d, info size: %d writing to pcm index %d",
                         outputIndex,
                         outputSize,
                         info.size,
                         m_writeIndex);*/

                    // copy the data out of the buffer
                    memcpy(decodedData + bytesWritten, outputBuffer, info.size);
                    bytesWritten += info.size;
                    AMediaCodec_releaseOutputBuffer(codec, outputIndex, false);
                } else {

                    // The outputIndex doubles as a status return if its value is < 0
                    switch (outputIndex) {
                        case AMEDIACODEC_INFO_TRY_AGAIN_LATER:
                            LOGD("dequeueOutputBuffer: try again later");
                            break;
                        case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                            LOGD("dequeueOutputBuffer: output buffers changed");
                            break;
                        case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
                            LOGD("dequeueOutputBuffer: output outputFormat changed");
                            format = AMediaCodec_getOutputFormat(codec);
                            LOGD("outputFormat changed to: %s", AMediaFormat_toString(format));
                            break;
                    }
                }
            }
        }

        *output = (AAssetDataSource*)malloc(sizeof(AAssetDataSource));
        (*output)->size = bytesWritten / sizeof(int16_t);
        (*output)->buffer = (int16_t *) decodedData;
    }
error:
    AMediaFormat_delete(format);
    AMediaCodec_delete(codec);
    AMediaExtractor_delete(extractor);
    AAsset_close(asset);
    return AMEDIA_OK;
}

static aaudio_data_callback_result_t audio_callback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames) {
//    float *floatData = (float *) audioData;
//    for (int i = 0; i < numFrames; ++i) {
//        float sampleValue = kAmplitude * sinf(mPhase);
//        for (int j = 0; j < kChannelCount; j++) {
//            floatData[i * kChannelCount + j] = sampleValue;
//        }
//        mPhase += mPhaseIncrement;
//        if (mPhase >= kTwoPi) mPhase -= kTwoPi;
//    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

    // Call this from Activity onResume()
static int32_t startAudio() {
    //std::lock_guard<std::mutex> lock(mLock);

    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAUDIO_OK;
    result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudio_createStreamBuilder, err %d", result);
        goto error;
    }
    //AAudioStreamBuilder_setDeviceId(builder, deviceId);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setSampleRate(builder, kSampleRate);
    AAudioStreamBuilder_setChannelCount(builder, kChannelCount);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    //AAudioStreamBuilder_setBufferCapacityInFrames(builder, frames);
    //AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_MEDIA);
    AAudioStreamBuilder_setDataCallback(builder, audio_callback, NULL);

    result = AAudioStreamBuilder_openStream(builder, &stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStreamBuilder_openStream, err %d", result);
        goto error;
    }

//            // The builder set methods can be chained for convenience.
//            oboe::Result result = builder.setSharingMode(oboe::SharingMode::Exclusive)
//                    ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
//                    ->setChannelCount(kChannelCount)
//                    ->setSampleRate(kSampleRate)
//                    ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
//                    ->setFormat(oboe::AudioFormat::Float)
//                    ->setDataCallback(this)
//                    ->openStream(mStream);

//if (result != AAUDIO_OK) return (int32_t) result;

    // Typically, start the stream after querying some stream information, as well as some input from the user

    result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) {
        LOGE("Error AAudioStream_requestStart, err %d", result);
        goto error;
    }
error:
    aaudio_result_t delete_result = AAudioStreamBuilder_delete(builder);
    if (delete_result != AAUDIO_OK) {
        LOGE("Error AAudioStreamBuilder_delete, err %d", delete_result);
    }
    return (int32_t) result;
}

// Call this from Activity onPause()
static void stopAudio() {
    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
}

