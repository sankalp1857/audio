/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * Android AudioRecord to read-only SnsrStream adapter.
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr;

import java.io.IOException;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.MediaRecorder.AudioSource;

import com.sensory.speech.snsr.SnsrStream;


/*
 * Implements the SnsrStream.Provider interface for live audio.
 *
 * Create a new SnsrStream instance with:
 * SnsrStream a = SnsrStream.fromProvider(new SnsrStreamAudioDeviceAndroid(16000),
 *                                        SnsrStreamMode.READ);
*/
class SnsrStreamAudioDeviceAndroid implements SnsrStream.Provider {
    private static final String TAG = "SnsrStreamAudioDeviceAndroid";
    private static final int CHANNELS = AudioFormat.CHANNEL_IN_MONO;
    private static final int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
    private int mSource = AudioSource.VOICE_RECOGNITION;
    /* Use an audio buffer that is at least this long */
    private static final int MIN_BUFFER_SIZE_MS = 1000;
    private AudioRecord mAudio;
    private int mBufferSize;
    private int mSampleRate;

    /**
     * Static constructor.
     * @param[in] sampleRate the sample rate. Use 16000.
     */
    public SnsrStreamAudioDeviceAndroid(int source, int sampleRate) {
        double minBufferSize = (double)sampleRate * MIN_BUFFER_SIZE_MS / 1000;
        mBufferSize =
          AudioRecord.getMinBufferSize(sampleRate, CHANNELS, ENCODING);
        if (mBufferSize < minBufferSize) {
          mBufferSize =
            mBufferSize * (int)Math.ceil(minBufferSize / mBufferSize);
        }
        mSampleRate = sampleRate;
        mSource = source;
    }

    /**
     * Static constructor with VOICE_RECOGNITION source.
     * @param[source] recording source.
     * @param[in] sampleRate the sample rate. Use 16000.
     */
    public SnsrStreamAudioDeviceAndroid(int sampleRate) {
      this(AudioSource.VOICE_RECOGNITION, sampleRate);
    }

    @Override
    public long onOpen() throws IOException {
        mAudio = new AudioRecord(mSource, mSampleRate,
                                 CHANNELS, ENCODING, mBufferSize);
        if (mAudio == null ||
            mAudio.getState() != AudioRecord.STATE_INITIALIZED) {
          mAudio = null;
          throw new IOException("Could not initialize audio device at " +
                                mSampleRate + " Hz.");
        }
        try {
          mAudio.startRecording();
        } catch (IllegalStateException e) {
          mAudio = null;
          throw new IOException(e.toString());
        }
        return OK;
    }

    @Override
    public long onClose() throws IOException {
        try {
            mAudio.stop();
        } catch (IllegalStateException e) {
          // ignore
        }
        mAudio.release();
        mAudio = null;
        return OK;
    }

    @Override
    public void onRelease() {
        if (mAudio != null) mAudio.release();
        mAudio = null;
    }

    @Override
    public long onRead(byte[] buffer) throws IOException {
      int read = mAudio.read(buffer, 0, buffer.length);
      if (Thread.interrupted()) return INTERRUPTED;
      if (read == AudioRecord.ERROR_BAD_VALUE) return INVALID_ARG;
      else if (read < 0) return ERROR;
      return read;
    }

    @Override
    public long onWrite(byte[] buffer) throws IOException {
      return NOT_IMPLEMENTED;
    }
}
