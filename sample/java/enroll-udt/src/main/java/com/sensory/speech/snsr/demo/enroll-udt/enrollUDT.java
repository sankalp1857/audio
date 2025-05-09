/* Sensory Confidential
 * Copyright (C)2016-2025 Sensory, Inc. https://sensory.com/
 *
 * Command-line User-Defined Trigger enrollment.
 *------------------------------------------------------------------------------
 */

import java.io.Console;
import java.io.IOException;
import com.sensory.speech.snsr.Snsr;
import com.sensory.speech.snsr.SnsrDataFormat;
import com.sensory.speech.snsr.SnsrRC;
import com.sensory.speech.snsr.SnsrSession;
import com.sensory.speech.snsr.SnsrStream;

import enroll.BuildConfig;

public class enrollUDT {
  public static void main(String argv[]) {
    final int SAMPLE_RATE = 16000;
    final String SpeakNow = "\nPlease say your enrollment phrase";
    final String EnrollmentContext =
      BuildConfig.MODEL_DIR + "/enrollment-context.snsr";
    String userTag = "custom-phrase";

    if (argv.length == 1) userTag = argv[0];
    else if (argv.length != 0) {
      System.out.println("usage: ./gradlew enroll [-Ptag=user-or-phrase-tag]");
      System.exit(7);
    }

    // Live audio stream handle.
    SnsrStream audio = SnsrStream.fromAudioDevice();

    // Primary TrulyHandsfree session handle.
    SnsrSession s = new SnsrSession();
    try {
      s.load(BuildConfig.UDT_MODEL).require(Snsr.TASK_TYPE, Snsr.ENROLL);
    } catch (IOException e) {
      e.printStackTrace();
      System.exit(3);
    }

    try {
      s.load(EnrollmentContext);
      try {
        s.setString(Snsr.DELETE_USER, userTag);
      } catch (Exception e) {}
      System.out.println("Loaded enrollments from " + EnrollmentContext);
      s.forEach(Snsr.USER_LIST, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            System.out.println("User " + s.getString(Snsr.USER)
                               + " has " + s.getInt(Snsr.RES_ENROLLMENT_COUNT)
                               + " enrollments.");
            return SnsrRC.OK;
          }
        });
    } catch (IOException e) {
      // ignore
    }

    s.setStream(Snsr.SOURCE_AUDIO_PCM, audio)
      .setString(Snsr.USER, userTag)
      .setHandler(Snsr.FAIL_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            System.out.println("This enrollment recording is not usable.");
            System.out.println(" Reason: " + s.getString(Snsr.RES_REASON));
            System.out.println("    Fix: " + s.getString(Snsr.RES_GUIDANCE));
            return SnsrRC.OK;
          }
        })
      .setHandler(Snsr.PASS_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            System.out.println("Recording passes preliminary tests.");
            return SnsrRC.OK;
          }
        })
      .setHandler(Snsr.PROG_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            double p = s.getDouble(Snsr.RES_PERCENT_DONE);
            System.out.print(String.format("\rAdapting: %3.0f%% done.    ", p));
            if (p >= 100) System.out.println("");
            return SnsrRC.OK;
          }
        })
      .setHandler(Snsr.PAUSE_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            // Pause recording while processing.
            System.out.println("");
            audio.close();
            return SnsrRC.OK;
          }
        })
      .setHandler(Snsr.RESUME_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            try {
              // Restart recording.
               audio.open();
            } catch (Exception e) {
              e.printStackTrace();
            }
            String prompt = SpeakNow + " ("
              + (s.getInt(Snsr.RES_ENROLLMENT_COUNT) + 1) + "/"
              + s.getInt(Snsr.ENROLLMENT_TARGET) + ")";
            if (s.getInt(Snsr.ADD_CONTEXT) != 0) {
              prompt += " with context,\n for example: " +
                "\"<phrase> will it rain tomorrow?\"";
            }
            System.out.println(prompt);
            return SnsrRC.OK;
          }
        })
      .setHandler(Snsr.DONE_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            SnsrStream out =
              SnsrStream.fromFileName(BuildConfig.ENROLLED_MODEL, "w");
            try {
              out.copy(s.getStream(Snsr.MODEL_STREAM));
              System.out.println("Enrolled model saved to "
                                 + BuildConfig.ENROLLED_MODEL);
            } catch (Exception e) {
              e.printStackTrace();
            }
            out.close();
            System.out.println("Done!");
            return SnsrRC.STOP;
          }
        })
      // Optional: save enrollment context
      // Use Snsr.ENROLLED_EVENT to save the
      // unadapted enrollment context instead.
      .setHandler(Snsr.ADAPTED_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            s.save(SnsrDataFormat.RUNTIME, EnrollmentContext);
            System.out.println("Enrollment context saved to "
                               + EnrollmentContext);
            return SnsrRC.OK;
          }
        })
      // Show audio recording duration
      .setHandler(Snsr.SAMPLES_EVENT, new SnsrSession.Listener() {
          public SnsrRC onEvent(SnsrSession s, String key) {
            double count = s.getDouble(Snsr.RES_SAMPLES);
            System.out.print(String.format("\rRecording: %6.2f s          ",
                                           count / SAMPLE_RATE));
            return SnsrRC.OK;
          }
        });

    try {
      s.run();
      // Optional but good practice. finalize() will (eventually) release.
      s.release();
      audio.release();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
}
