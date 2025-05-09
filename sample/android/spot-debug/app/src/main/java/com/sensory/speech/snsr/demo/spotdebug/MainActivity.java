/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr.demo.spotdebug;

import android.Manifest;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.sensory.speech.snsr.Snsr;

import java.io.File;

@SuppressWarnings("unused")
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final String TriggerPhrase = "Hello Blue Genie";
    private static final int TALK = 2;
    private Button mButtonTalk, mButtonStop;
    private TextView mTvConsole, mTvState;
    private ScrollView mScroll;
    private CheckBox mCheckboxDebug;
    private PhraseSpot mPhraseSpot;
    private File mLogFile;
    private int mOnResume = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mButtonTalk = (Button) findViewById(R.id.buttonTalk);
        mButtonStop = (Button) findViewById(R.id.buttonStop);

        mTvConsole = (TextView) findViewById(R.id.textViewConsole);
        mTvState = (TextView) findViewById(R.id.textViewState);

        mScroll = (ScrollView) findViewById(R.id.scrollView);
        mCheckboxDebug = (CheckBox) findViewById(R.id.checkBoxDebug);
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
        else setUiState(UiState.NOT_TALKING);
        mOnResume = 0;
    }

    private void setUiState(UiState uiState) {
        switch (uiState) {
            case NOT_TALKING:
                setEnabledLook(mButtonTalk, true);
                setEnabledLook(mButtonStop, false);
                setEnabledLook(mCheckboxDebug, true);
                mTvState.setText("");
                break;
            case TALKING:
                setEnabledLook(mButtonTalk, false);
                setEnabledLook(mButtonStop, true);
                setEnabledLook(mCheckboxDebug, false);
                mTvState.setText(R.string.listening_trigger);
                break;
            default:
                Log.e("TAG", "Unsupported UI state: " + uiState);
        }
    }

    private void setEnabledLook(View view, boolean enabled) {
        view.setEnabled(enabled); // Button can be null here on startup?
        if (enabled)
            view.setAlpha(1.0f);
        else
            view.setAlpha(0.5f);
    }

    @SuppressWarnings("UnusedParameters")
    public void onButtonTalk(View view) {
        requestRuntimePermission(TALK);
    }

    private void startTask(int task) {
        if (task != TALK) return;
        logToConsole("Say " + TriggerPhrase + " among other speech.");
        String triggerPath =
                new File("assets/models", BuildConfig.TRIGGER.replace(':', '-')).toString();
        mPhraseSpot = new PhraseSpot(this, triggerPath, 30);
        if (mCheckboxDebug.isChecked()) {
            String debugTemplatePath =
                    new File("assets/models", BuildConfig.DEBUG_TEMPLATE).toString();
            // Don't forget to have permissions to write to external storage in manifest
            // <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
            File base = getExternalFilesDir(null);
            File sensoryFolder = new File(base, "Sensory/Logs");
            sensoryFolder.mkdirs();
            // Create a reasonable, unique debug file name
            String appName = getString(R.string.app_name);
            long sec = System.currentTimeMillis() / 1000;
            mLogFile = new File(sensoryFolder, appName + "-" + sec + ".log");
            Log.i(TAG, "Logfile will be: " + mLogFile);
            mPhraseSpot.enableDebugging(debugTemplatePath, mLogFile.toString());
        } else {
            mLogFile = null;
        }
        mPhraseSpot.start();
        setUiState(UiState.TALKING);
    }

    @SuppressWarnings("UnusedParameters")
    public void onButtonStop(View view) {
        stopSpeechActivities();
        logToConsole("Stopped by user.");
        notify(UiState.NOT_TALKING);
        if (mLogFile != null) {
            // Help make our new logfile show up.  If you still can't browse for example
            // Sensory/Logs/<your_log_file>.log
            // you can replug the device on USB or even try power-cycling it.
            sendBroadcast(
                    new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, Uri.fromFile(mLogFile)));
        }
    }

    void logToConsole(final String text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                appendToConsole(text);
            }
        });
    }

    private void appendToConsole(String text) {
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
        setUiState(uiNotify);
    }

    private void stopSpeechActivities() {
        if (mPhraseSpot != null) {
            mPhraseSpot.stop();
            mPhraseSpot = null;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.i(TAG, "onRequestPermissionResult()");
        if (grantResults.length != 2) return;
        if (grantResults[0] != PackageManager.PERMISSION_GRANTED || grantResults[1] != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this, "Audio recording and SD card access permissions are required.", Toast.LENGTH_LONG).show();
            mOnResume = 0;
            // finish();
        } else {
            mOnResume = requestCode;
        }
    }


    protected void requestRuntimePermission(int task) {
        int currentapiVersion = android.os.Build.VERSION.SDK_INT;
        if (currentapiVersion > android.os.Build.VERSION_CODES.LOLLIPOP &&
                (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED ||
                        ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)) {
            ActivityCompat.requestPermissions(this,
                    new String[]{
                            Manifest.permission.RECORD_AUDIO,
                            Manifest.permission.WRITE_EXTERNAL_STORAGE
                    }, task);
        } else {
            startTask(task);
        }
    }

}
