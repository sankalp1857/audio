/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 * Command-line phrase spotter, runs trailing audio through VAD
 * and saves this speech-detected audio to file.
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

public class segmentSpottedAudio {
  public static void main(String argv[]) {
    final String VAD_AUDIO_FILE = "vad-audio.wav";
    final Boolean INCLUDE_SPOT = (argv.length == 1);

    // Spot from live audio
    SnsrStream audio = SnsrStream.fromAudioDevice();

    // Primary TrulyHandsfree session handle
    SnsrSession s = new SnsrSession();
    try {
      // Load and validate the spot-vad template model.
      s.load(BuildConfig.VAD_TEMPLATE);
      s.require(Snsr.TASK_TYPE, Snsr.PHRASESPOT_VAD);
      // Fill in slot #0 with a phrase spotter.
      s.setStream(Snsr.SLOT_0,
                  SnsrStream.fromFileName(BuildConfig.HBG_MODEL, "r"));
    } catch (IOException e) {
      e.printStackTrace();
      System.exit(3);
    }

    // Output file for VAD-selected audio.
    SnsrStream out = SnsrStream.fromAudioFile(VAD_AUDIO_FILE, "w");

    // Configure session
    s .setStream(Snsr.SOURCE_AUDIO_PCM, audio)
      .setStream(Snsr.SINK_AUDIO_PCM, out)
      .setInt(Snsr.INCLUDE_LEADING_SILENCE, INCLUDE_SPOT? 1: 0)
      .setInt(Snsr.BACKOFF, 0)    // reduce VAD audio margins to the minimum
      .setInt(Snsr.HOLD_OVER, 0)
      // Phrase spot event.
      .setHandler(Snsr.RESULT_EVENT, (session, key) -> {
          System.out.println(String.format("Found \"%s\"... listening",
                                           session.getString("text")));
          return SnsrRC.OK;
        });

    // VAD endpoint callback
    SnsrSession.Listener endpoint = new SnsrSession.Listener() {
        public SnsrRC onEvent(SnsrSession s, String key) {
          double from = s.getDouble(Snsr.RES_BEGIN_MS);
          double to   = s.getDouble(Snsr.RES_END_MS);
          final String msg =
            String.format("Found speech from %.3f ms to %.3f ms", from, to);
          System.out.println("VAD endpoint " + key + "\n" + msg);
          // Stop after one VAD endpoint detection.
          return SnsrRC.STOP;
        }
      };
    // Wire up handlers for the VAD events.
    s .setHandler(Snsr.END_EVENT, endpoint)
      .setHandler(Snsr.LIMIT_EVENT, endpoint)
      .setHandler(Snsr.BEGIN_EVENT, (session, key) -> {
          System.out.println("VAD start detected.");
          return SnsrRC.OK;
      })
      .setHandler(Snsr.SILENCE_EVENT, (session, key) -> {
          System.out.println("VAD endpoint " + key + "\n" +
                             "Listening for \"Hello Blue Genie\".");
          return SnsrRC.OK;
        });

    // Show an SDK license expiration warning, if needed
    final String licenseWarning = s.getString(Snsr.LICENSE_WARNING);
    if (licenseWarning != null) System.out.println(licenseWarning);

    System.out.println("Say: \"Hello Blue Genie will it rain "
                       + "in Portland tomorrow?\"");
    try {
      s.run();
    } catch (IOException e) {
      e.printStackTrace();
      System.exit(4);
    }
    s.release();
    out.release();
    System.out.println("Wrote recording to \"" + VAD_AUDIO_FILE + "\"");
  }
}
