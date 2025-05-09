/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * Command-line phrase spotter.
 *------------------------------------------------------------------------------
 */

import java.io.Console;
import java.io.File;
import java.io.IOException;

import com.sensory.speech.snsr.Snsr;
import com.sensory.speech.snsr.SnsrRC;
import com.sensory.speech.snsr.SnsrSession;
import com.sensory.speech.snsr.SnsrStream;

import enroll.BuildConfig;

public class evalUDT {
  public static void main(String argv[]) {
    final int TIMEOUT = 60;
    final int SAMPLE_RATE = 16000;

    // Check whether the enrolled spotter model exists
    if (!(new File(BuildConfig.ENROLLED_MODEL).exists())) {
      System.out.println("Enrollment model file " + BuildConfig.ENROLLED_MODEL);
      System.out.println("was not found. Please enroll a phrase by running:" +
                         "./gradlew -q enroll");
      System.exit(1);
    }

    // Holder for the number of spots encountered so far
    final int[] spotCount = new int[1];

    // Spot from live audio
    SnsrStream audio = SnsrStream.fromAudioDevice();

    // Primary TrulyHandsfree session handle
    SnsrSession s = new SnsrSession();
    try {
      s.load(BuildConfig.ENROLLED_MODEL);
    } catch (IOException e) {
      e.printStackTrace();
      System.exit(3);
    }

    s.require(Snsr.TASK_TYPE, Snsr.PHRASESPOT)
      // .setDouble(Snsr.SV_THRESHOLD, 0.1)  // test - override default
      .setStream(Snsr.SOURCE_AUDIO_PCM, audio)
      // Show the duration of processed audio,
      // and stop after TIMEOUT seconds
      .setHandler(Snsr.SAMPLES_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            double count = s.getDouble(Snsr.RES_SAMPLES);
            System.out.print(String.format("\rRecording: %6.2f s",
                                           count / SAMPLE_RATE));
            if (count < SAMPLE_RATE * TIMEOUT) return SnsrRC.OK;
            return SnsrRC.TIMED_OUT;
          }
        })
      // Phrase spot event. Show speaker verification score and alignments.
      .setHandler(Snsr.RESULT_EVENT, new SnsrSession.Listener() {
        public SnsrRC onEvent(SnsrSession s, String key) {
          System.out.println(String.format("\r#%02d \"%s\", score = %.3f",
                                           spotCount[0]++,
                                           s.getString("text"),
                                           s.getDouble("sv-score")));
          // Replace Snsr.WORD_LIST with Snsr.PHONE_LIST to show phonemes
          s.forEach(Snsr.WORD_LIST, new SnsrSession.Listener() {
              public SnsrRC onEvent(SnsrSession s, String key) {
                System.out.println(String.format("  [%.0f ms, %.0f ms] %s",
                                                 s.getDouble(Snsr.RES_BEGIN_MS),
                                                 s.getDouble(Snsr.RES_END_MS),
                                                 s.getString(Snsr.RES_TEXT)));
                return SnsrRC.OK;
              }
            });
          System.out.println("");
          return SnsrRC.OK;
        }
        });

    // Show an SDK license expiration warning, if needed
    final String licenseWarning = s.getString(Snsr.LICENSE_WARNING);
    if (licenseWarning != null) System.out.println(licenseWarning);

    System.out.println("Say your enrolled phrase.");
    try {
      s.run();
      s.release();
      audio.release();
    } catch (IOException e) {
      e.printStackTrace();
    }
    System.out.println("\nDone.");
  }
}
