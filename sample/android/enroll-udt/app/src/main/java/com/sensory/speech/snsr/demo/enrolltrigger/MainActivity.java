/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr.demo.enrolltrigger;

import android.Manifest;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.sensory.speech.snsr.Snsr;

import java.io.File;

@SuppressWarnings("unused")
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final String ENROLL_UDT_MODEL = "assets/models/udt-universal.snsr";
    private static final String EnrollOutFile = "enrolled.snsr";
    private static final String TriggerPhrase = "custom-trigger-phrase";
    private static final int ENROLL = 1;
    private static final int TALK = 2;
    private Button mButtonEnroll, mButtonTalk, mButtonStop;
    private TextView mTvConsole, mTvState;
    private ScrollView mScroll;
    private String mEnrollFile;
    private Enroll mEnroll;
    private PhraseSpot mPhraseSpot;
    private int mOnResume = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mButtonEnroll = (Button) findViewById(R.id.buttonEnroll);
        mButtonTalk = (Button) findViewById(R.id.buttonTalk);
        mButtonStop = (Button) findViewById(R.id.buttonStop);

        mTvConsole = (TextView) findViewById(R.id.textViewConsole);
        mTvState = (TextView) findViewById(R.id.textViewState);

        mScroll = (ScrollView) findViewById(R.id.scrollView);
        // Stay in portrait mode even if user tilts
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        Snsr.init(this);  // Required for direct access to assets

        if (savedInstanceState == null) {
            // New application instance
            mTvConsole.setText("");
        }
    }


    @Override
    public void onPause() {
        Log.i(TAG, "onPause()");
        stopSpeechActivities();
        super.onPause();
    }

    @Override
    protected void onResume() {
        Log.i(TAG, "onResume()");
        super.onResume();
        if (mOnResume > 0) startTask(mOnResume);
        else setUiState(UiState.BEFORE_ENROLL);
        mOnResume = 0;
    }

    private void setUiState(UiState uiState) {
        switch (uiState) {
            case BEFORE_ENROLL:
                setEnabledLook(mButtonEnroll, true);
                File enrollOutDir = Enroll.getOutDir(this);
                File enrollOutFile = new File(enrollOutDir, EnrollOutFile);
                boolean canTalk = enrollOutFile.exists();
                if (canTalk)
                    mEnrollFile = enrollOutFile.toString();
                setEnabledLook(mButtonTalk, canTalk);
                setEnabledLook(mButtonStop, false);
                mTvState.setText("");
                break;
            case ENROLLING:
                setEnabledLook(mButtonEnroll, false);
                setEnabledLook(mButtonTalk, false);
                setEnabledLook(mButtonStop, true);
                mTvState.setText(R.string.enrolling_trigger);
                break;
            case ENROLLED:
                setEnabledLook(mButtonEnroll, true);
                setEnabledLook(mButtonTalk, true);
                setEnabledLook(mButtonStop, false);
                mTvState.setText("");
                break;
            case TALKING:
                mTvState.setText(R.string.listening_trigger);
                setEnabledLook(mButtonEnroll, false);
                setEnabledLook(mButtonTalk, false);
                setEnabledLook(mButtonStop, true);
                break;
            default:
                Log.e("TAG", "Unsupported UI state: " + uiState);
        }
    }

    private void setEnabledLook(Button button, boolean enabled) {
        button.setEnabled(enabled); // Button can be null here on startup?
        if (enabled)
            button.setAlpha(1.0f);
        else
            button.setAlpha(0.5f);
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.i(TAG, "onRequestPermissionResult()");
        if (grantResults.length == 0) return;
        if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Audio record permission is required.", Toast.LENGTH_LONG).show();
            mOnResume = 0;
            // finish();
        } else {
            mOnResume = requestCode;
        }
    }


    protected void requestRecordingPermission(int task) {
        int currentapiVersion = android.os.Build.VERSION.SDK_INT;
        if (currentapiVersion > android.os.Build.VERSION_CODES.LOLLIPOP &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO}, task);
        } else {
            startTask(task);
        }
    }


    private void startTask(int task) {
        switch (task) {
            case ENROLL:
                startEnrollment();
                break;
            case TALK:
                startTalking();
                break;
        }
    }


    private void startEnrollment() {
        mEnroll = new Enroll(this, TriggerPhrase, ENROLL_UDT_MODEL, EnrollOutFile);
        mEnroll.start();
        setUiState(UiState.ENROLLING);
    }


    private void startTalking() {
        log("Say " + TriggerPhrase + " among other speech.");
        mPhraseSpot = new PhraseSpot(this, mEnrollFile, 30);
        mPhraseSpot.start();
        setUiState(UiState.TALKING);
    }


    @SuppressWarnings("UnusedParameters")
    public void onButtonEnroll(View view) {
        requestRecordingPermission(ENROLL);
    }

    @SuppressWarnings("UnusedParameters")
    public void onButtonTalk(View view) {
        requestRecordingPermission(TALK);
    }

    @SuppressWarnings("UnusedParameters")
    public void onButtonStop(View view) {
        stopSpeechActivities();
        log("Stopped by user.");
        notify(UiState.BEFORE_ENROLL);
    }

    void log(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                appendToLog(text);
            }
        });
    }

    private void appendToLog(String text) {
        Log.i(TAG, "Console: " + text);
        mTvConsole.append(text + "\n");
        mScroll.post(new Runnable() {
            @Override
            public void run() {
                mScroll.fullScroll(View.FOCUS_DOWN);
            }
        });
    }

    void notify(final UiState uiState) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                handleNotify(uiState);
            }
        });
    }

    private void handleNotify(UiState uiNotify) {
        if (uiNotify == UiState.ENROLLED) {
            mEnrollFile = mEnroll.getOutPath();
            mEnroll = null;
            log("- - Done! - -");
        }
        setUiState(uiNotify);
    }

    private void stopSpeechActivities() {
        if (mEnroll != null) {
            mEnroll.stop();
            mEnroll = null;
        }
        if (mPhraseSpot != null) {
            mPhraseSpot.stop();
            mPhraseSpot = null;
        }
    }
}
