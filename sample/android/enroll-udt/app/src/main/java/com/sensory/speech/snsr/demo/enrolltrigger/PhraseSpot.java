/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr.demo.enrolltrigger;

import android.util.Log;

import com.sensory.speech.snsr.Snsr;
import com.sensory.speech.snsr.SnsrRC;
import com.sensory.speech.snsr.SnsrSession;
import com.sensory.speech.snsr.SnsrStream;

import java.io.IOException;
import java.util.Locale;

@SuppressWarnings({"SameParameterValue", "CanBeFinal", "UnusedReturnValue"})
class PhraseSpot implements SnsrSession.Listener {
    private final String TAG = "PhraseSpot";
    private Thread mRecogThread;
    private String mModelPath;
    private double mTimeout = 0.0;
    private int mSampleRate;
    private double mSamples;
    private double mSamplesTimeoutBegin;
    private MainActivity mUi;

    PhraseSpot(MainActivity mainActivity, String model, double timeout) {
        mUi = mainActivity;
        mModelPath = model;
        mTimeout = timeout;
        mSamples = mSamplesTimeoutBegin = 0;
    }

    public synchronized void start() {
        if (mRecogThread == null) {
            Log.d(TAG, "Starting recognition thread.");
            mRecogThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    doPhraseSpot();
                }
            });
            mRecogThread.start();
        }
    }

    public synchronized void stop() {
        if (mRecogThread != null && mRecogThread.isAlive()) {
            Log.d(TAG, "Stopping recognition thread.");
            mRecogThread.interrupt();
            try {
                mRecogThread.join();
                mRecogThread = null;
            } catch (InterruptedException e) {
                /* ignore */
            }
        }
    }

    private SnsrRC doPhraseSpot() {
        Log.d(TAG, "Loading from " + mModelPath + "\n");
        SnsrStream audio = SnsrStream.fromAudioDevice();
        // Could also chain these, like so:
        // SnsrSession session = new SnsrSession().load(mModelPath).require(..).setStream(..).setHandler(..)
        SnsrSession session = new SnsrSession();
        try {
          session.load(mModelPath);
          session.require(Snsr.TASK_TYPE, Snsr.PHRASESPOT);
          session.setStream(Snsr.SOURCE_AUDIO_PCM, audio);
          session.setHandler(Snsr.RESULT_EVENT, this);

          // In case timeout set
          mSampleRate = session.getInt(Snsr.SAMPLE_RATE);
          session.setHandler(Snsr.SAMPLES_EVENT, this);
          session.run();
        } catch (IOException e) {
          /* ignore */
        }
        this.onEvent(session, "stopped");

        SnsrRC rc = session.rC();
        // Release the underlying C handles immediately, rather than waiting for GC.
        session.release();
        audio.release();
        return rc;
    }

        @Override
    public SnsrRC onEvent(SnsrSession s, String key) {
        if (!Snsr.SAMPLES_EVENT.equals(key))
            Log.i(TAG, "SNSR Event: " + key);
        switch (key) {
            case Snsr.SAMPLES_EVENT:
                if (mTimeout == 0) return SnsrRC.OK;
                mSamples = s.getDouble(Snsr.RES_SAMPLES);
                double elapsedSamples = mSamples - mSamplesTimeoutBegin;
                if (elapsedSamples > mTimeout * mSampleRate) {
                    mUi.log("Phrase spot timed out.");
                    return SnsrRC.TIMED_OUT;
                }
                else
                    return SnsrRC.OK;
            case Snsr.RESULT_EVENT:
                // Start timeout all over again when we hear the trigger
                mSamplesTimeoutBegin = mSamples;
                mUi.log(String.format(Locale.US, "\"%s\", score: %.3f",
                        s.getString(Snsr.RES_TEXT), s.getDouble(Snsr.RES_SV_SCORE)));
                // Try changing this to Snsr.PHONE_LIST
                s.forEach(Snsr.WORD_LIST, new SnsrSession.Listener() {
                    @Override
                    public SnsrRC onEvent(SnsrSession s, String key) {
                        mUi.log(String.format(Locale.US, "  [%4.0f, %4.0f] %s\n",
                                s.getDouble(Snsr.RES_BEGIN_MS),
                                s.getDouble(Snsr.RES_END_MS),
                                s.getString(Snsr.RES_TEXT)));
                        return SnsrRC.OK;
                    }
                });
                return SnsrRC.OK;
            case "stopped":
                mUi.notify(UiState.BEFORE_ENROLL);
                return SnsrRC.OK;
            default:
                Log.e(TAG, "Failed to implement handler for: "+key);
                return SnsrRC.OK;
        }
    }
}
