/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * Generic audio recording to read-only SnsrStream adapter.
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr;

import java.io.IOException;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Line;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.Mixer;
import javax.sound.sampled.TargetDataLine;

import com.sensory.speech.snsr.SnsrStream;


/*
 * Implements the SnsrStream.Provider interface for live audio.
 *
 * Create a new SnsrStream instance with:
 * SnsrStream a = SnsrStream.fromProvider(new SnsrStreamAudioDeviceGeneric(16000),
 *                                      SnsrStreamMode.READ);
 */
class SnsrStreamAudioDeviceGeneric implements SnsrStream.Provider {
    private TargetDataLine mInput;
    private AudioFormat mAudioFormat;

    public SnsrStreamAudioDeviceGeneric(int sampleRate) {
        mInput = getDefaultMicrophone();
        mAudioFormat = new AudioFormat((float) sampleRate, 16, 1, true, false);
    }

    @Override
    public long onOpen() throws IOException {
        if (mInput == null) return NOT_OPEN;
        try {
            mInput.open(mAudioFormat);
            mInput.start();
        } catch (LineUnavailableException e) {
            throw new IOException(e.toString());
        }
        return OK;
    }

    @Override
    public long onClose() throws IOException {
        mInput.stop();
        mInput.close();
        return OK;
    }

    @Override
    public void onRelease() {
        mInput = null;
    }

    @Override
    public long onRead(byte[] buffer) throws IOException {
        long read = mInput.read(buffer, 0, buffer.length);
        if (Thread.interrupted()) return INTERRUPTED;
        return read;
    }

    @Override
    public long onWrite(byte[] buffer) throws IOException {
        return NOT_IMPLEMENTED;
    }

    /* Find the default system microphone */
    private TargetDataLine getDefaultMicrophone() {
        Mixer.Info[] mixers = AudioSystem.getMixerInfo();
        for (Mixer.Info mixerInfo : mixers) {
            Mixer m = AudioSystem.getMixer(mixerInfo);
            try {
                m.open();
                m.close();
            } catch (Exception e) {
                continue;
            }
            Line.Info[] lines = m.getTargetLineInfo();
            for (Line.Info l : lines) {
                try {
                    TargetDataLine t = (TargetDataLine) AudioSystem.getLine(l);
                    if (t!= null) return t;
                } catch (Exception e) {
                  /* ignore */
                }
            }
        }
        return null;
    }
}
