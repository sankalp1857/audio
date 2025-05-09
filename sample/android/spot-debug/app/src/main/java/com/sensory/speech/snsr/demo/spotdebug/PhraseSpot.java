/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr.demo.spotdebug;

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
    private String mModelPath, mLogPath, mDebugTemplatePath;
    private double mTimeout = 0.0;
    private int mSampleRate;
    private double mSamples;
    private double mSamplesTimeoutBegin;
    private MainActivity mUi;
    private boolean mDebugging = false;
    private volatile boolean mRunning = false;

    PhraseSpot(MainActivity mainActivity, String model, double timeout) {
        mUi = mainActivity;
        mModelPath = model;
        mTimeout = timeout;
        mSamples = mSamplesTimeoutBegin = 0;
        mLogPath = null;
        mDebugging = false;
    }

    public void enableDebugging(String debugTemplate, String logPath) {
        mDebugging = true;
        mDebugTemplatePath = debugTemplate;
        mLogPath = logPath;
    }

    public synchronized void start() {
        if (mRecogThread == null) {
            mRunning = true;
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
            // mRecogThread.interrupt();
            mRunning = false;
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
        SnsrSession session = new SnsrSession();
        try {
            if (!mDebugging) {
                session.load(mModelPath);
            }
            else  {
                // In case of debugging, load debug container and load actual trigger into slot 0
                // On command line combining these into one debug snsr file would be:
                // snsr-edit -v -t spot-debug-1.snsr -f 0 <trigger file> -o spot-hbg-debug.snsr
                session.load(mDebugTemplatePath);
                session.setStream(Snsr.SLOT_0, SnsrStream.fromFileName(mModelPath,"r"));
                session.setString(Snsr.DEBUG_LOG_FILE, mLogPath);
            }

            // Could also chain these, like so:
            // SnsrSession session = new SnsrSession().load(mModelPath).require(..).setStream(..)
            session.require(Snsr.TASK_TYPE, Snsr.PHRASESPOT);
            session.setStream(Snsr.SOURCE_AUDIO_PCM, audio);
            session.setHandler(Snsr.RESULT_EVENT, this);

            // In case timeout was set
            mSampleRate = session.getInt(Snsr.SAMPLE_RATE);
            session.setHandler(Snsr.SAMPLES_EVENT, this);
            session.run();
        } catch (IOException e) {
            Log.e(TAG, "Error loading and starting model", e);
            mUi.logToConsole("ERROR: "+e.getMessage());
        }
        this.onEvent(session, "stopped");

        SnsrRC rc = session.rC();
        // Release the underlying native handles immediately, rather than waiting for GC.
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
                if (mRunning == false)
                    return SnsrRC.STOP;
                if (mTimeout == 0) return SnsrRC.OK;
                mSamples = s.getDouble(Snsr.RES_SAMPLES);
                double elapsedSamples = mSamples - mSamplesTimeoutBegin;
                if (elapsedSamples > mTimeout * mSampleRate) {
                    mUi.logToConsole("Phrase spot timed out.");
                    return SnsrRC.TIMED_OUT;
                }
                else
                    return SnsrRC.OK;
            case Snsr.RESULT_EVENT:
                // Start timeout all over again when we hear the trigger
                mSamplesTimeoutBegin = mSamples;
                mUi.logToConsole(String.format(Locale.US, "\n'%s'",
                        s.getString(Snsr.RES_TEXT)));
                // Try changing this to Snsr.PHONE_LIST
                s.forEach(Snsr.WORD_LIST, new SnsrSession.Listener() {
                    @Override
                    public SnsrRC onEvent(SnsrSession s, String key) {
                        mUi.logToConsole(String.format(Locale.US, "  [%4.0f, %4.0f] %s",
                                s.getDouble(Snsr.RES_BEGIN_MS),
                                s.getDouble(Snsr.RES_END_MS),
                                s.getString(Snsr.RES_TEXT)));
                        return SnsrRC.OK;
                    }
                });
                return SnsrRC.OK;
            case "stopped":
                mUi.notify(UiState.NOT_TALKING);
                return SnsrRC.OK;
            default:
                Log.e(TAG, "Failed to implement handler for: "+key);
                return SnsrRC.OK;
        }
    }
}
