/* Sensory Confidential
 * Copyright (C)2017-2025 Sensory, Inc. https://sensory.com/
 *
 * Unit tests for UDT enrollment and generated spotter tasks.
 *------------------------------------------------------------------------------
 */

package com.sensory.speech.snsr.test;

import java.io.IOException;
import java.util.*;

import org.junit.Test;
import static org.junit.Assert.*;
import org.junit.*;
import org.junit.runners.MethodSorters;

import com.sensory.speech.snsr.*;
import enroll.BuildConfig;


@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class SnsrEnrollmentTest {
  final String EnrollmentContext =
    BuildConfig.MODEL_DIR + "/test-enrollment-context.snsr";
  final String SvModel1 = BuildConfig.MODEL_DIR + "/test-1.snsr";
  final String SvModel2 = BuildConfig.MODEL_DIR + "/test-2.snsr";
  final String[] Users = {
    "armadillo-1", "jackalope-1"
  };
  final String[] TestUsers = {
    "armadillo-1", "armadillo-6",
    "jackalope-1", "jackalope-4",
    "terminator-2", "terminator-6"
  };


  // Enroll two users from file.
  // Save the adapted enrollment context to file.
  // This test has to run before enrollFromContext*
  @Test
  public void EnrollFromFile() {
    SnsrSession s = new SnsrSession();
    try {
      s .load(BuildConfig.UDT_MODEL)
        .require(Snsr.TASK_TYPE, Snsr.ENROLL)
        .setHandler(Snsr.DONE_EVENT, (session, key) -> {
            // Save enrolled model for further testing
            SnsrStream out = SnsrStream.fromFileName(SvModel2, "w");
            try {
              out.copy(session.getStream(Snsr.MODEL_STREAM));
              out.close();
            } catch (IOException e) {
              fail(e.toString());
            }
            System.out.println("Enrolled model saved to " + SvModel2);
            return SnsrRC.OK;
          })

        .setHandler(Snsr.PROG_EVENT, (session, key) -> {
            double p = session.getDouble(Snsr.RES_PERCENT_DONE);
            System.out.println(String.format("Adapting: %3.0f%% done.", p));
            return SnsrRC.OK;
          })

        // Prepare for non-interactive enrollment
        .setInt(Snsr.INTERACTIVE_MODE, 0);

      // Enroll example users
      for (String tag: Users) {
        s.setString(Snsr.USER, tag);
        for (int i = 0; i < 4; i++) {
          final String path =
            String.format("%s/%s-%d.wav", BuildConfig.ENROLLMENT_DIR, tag, i);
          SnsrStream a = SnsrStream.fromAudioFile(path, "r");
          s .setStream(Snsr.SOURCE_AUDIO_PCM, a)
            .run();
          assertEquals(SnsrRC.STREAM_END, s.rC());
          System.out.println("Enrolled " + tag + " with " + path);
        }
      }

      // List enrolled users
      s.forEach(Snsr.USER_LIST, (session, key) -> {
          System.out.println("Enrolled: " + session.getString(Snsr.USER));
          return SnsrRC.OK;
        });

      // End-of-enrollment markers
      s .setString(Snsr.USER, null)
        .setStream(Snsr.SOURCE_AUDIO_PCM, SnsrStream.fromString(""));
      assertEquals(SnsrRC.OK, s.rC());
      s.run();
      assertEquals(SnsrRC.STREAM_END, s.rC());

      // Save adapted enrollment context
      s.save(SnsrDataFormat.RUNTIME, EnrollmentContext);

    } catch (IOException e) {
      fail(e.toString());
    }

    s.release();
  }


  // Load the adapted enrollment context created by "enrollment" above.
  @Test
  public void enrollFromContext() {
    SnsrSession s = new SnsrSession();
    try {
      s .load(BuildConfig.UDT_MODEL)
        .require(Snsr.TASK_TYPE, Snsr.ENROLL)
        // Load the enrollment context after loading the primary model
        .load(EnrollmentContext)
        // Prepare for non-interactive enrollment
        .setInt(Snsr.INTERACTIVE_MODE, 0);

      // List enrolled users
      final List<String> enrolledUsers = new ArrayList<String>();
      s.forEach(Snsr.USER_LIST, (session, key) -> {
          enrolledUsers.add(session.getString(Snsr.USER));
          return SnsrRC.OK;
        });
      assertEquals(Arrays.toString(Users), enrolledUsers.toString());

      // End-of-enrollment markers
      s .setString(Snsr.USER, null)
        .setStream(Snsr.SOURCE_AUDIO_PCM, SnsrStream.fromString(""));
      assertEquals(SnsrRC.OK, s.rC());
      s.run();
      assertEquals(SnsrRC.STREAM_END, s.rC());

    } catch (IOException e) {
      fail(e.toString());
    }

    s.release();
  }


  // Load the adapted enrollment context created by "enrollment" above.
  // Remove one user, re-adapt.
  @Test
  public void enrollFromContextRemoveOne() {
    SnsrSession s = new SnsrSession();
    try {
      s .load(BuildConfig.UDT_MODEL)
        .require(Snsr.TASK_TYPE, Snsr.ENROLL)
        // Load the enrollment context after loading the primary model
        .load(EnrollmentContext)
        // Prepare for non-interactive enrollment
        .setInt(Snsr.INTERACTIVE_MODE, 0)
        // Save enrolled model for further testing
        .setHandler(Snsr.DONE_EVENT, (session, key) -> {
            SnsrStream out = SnsrStream.fromFileName(SvModel1, "w");
            try {
              out.copy(session.getStream(Snsr.MODEL_STREAM));
              out.close();
            } catch (IOException e) {
              fail(e.toString());
            }
            System.out.println("Enrolled model saved to " + SvModel1);
            return SnsrRC.OK;
          })

        // Remove the first enrollment
        .setString(Snsr.DELETE_USER, Users[0]);

      // List enrolled users
      final List<String> enrolledUsers = new ArrayList<String>();
      s.forEach(Snsr.USER_LIST, (session, key) -> {
          enrolledUsers.add(session.getString(Snsr.USER));
          return SnsrRC.OK;
        });
      assertEquals(Arrays.toString(Arrays.copyOfRange(Users, 1, Users.length)),
                   enrolledUsers.toString());

      // End-of-enrollment markers
      s .setString(Snsr.USER, null)
        .setStream(Snsr.SOURCE_AUDIO_PCM, SnsrStream.fromString(""));
      assertEquals(SnsrRC.OK, s.rC());
      s.run();
      assertEquals(SnsrRC.STREAM_END, s.rC());

    } catch (IOException e) {
      fail(e.toString());
    }

    s.release();
  }


  // Load the adapted enrollment context created by "enrollment" above.
  // Remove two users, re-adapt.
  @Test
  public void enrollFromContextRemoveTwo() {
    SnsrSession s = new SnsrSession();
    // Holder for DONE_EVENT count
    final int[] doneEventCount = new int[1];
    try {
      s .load(BuildConfig.UDT_MODEL)
        .require(Snsr.TASK_TYPE, Snsr.ENROLL)
        // Load the enrollment context after loading the primary model
        .load(EnrollmentContext)
        // Prepare for non-interactive enrollment
        .setInt(Snsr.INTERACTIVE_MODE, 0)
        .setHandler(Snsr.DONE_EVENT, (session, key) -> {
            // Invoked for each deleted user
            doneEventCount[0]++;
            return SnsrRC.STOP;
          });

      // Remove the first enrollment
      doneEventCount[0] = 0;
      s .setString(Snsr.DELETE_USER, Users[0])
        .setString(Snsr.DELETE_USER, Users[1]);
      assertEquals(SnsrRC.STOP, s.rC());
      assertEquals(doneEventCount[0], 2);

      // List enrolled users
      final List<String> enrolledUsers = new ArrayList<String>();
      s.forEach(Snsr.USER_LIST, (session, key) -> {
          enrolledUsers.add(session.getString(Snsr.USER));
          return SnsrRC.OK;
        });
      assertEquals(Arrays.toString(Arrays.copyOfRange(Users, 2, Users.length)),
                   enrolledUsers.toString());

      // End-of-enrollment markers
      s .setString(Snsr.USER, null)
        .setStream(Snsr.SOURCE_AUDIO_PCM, SnsrStream.fromString(""));
      assertEquals(SnsrRC.OK, s.rC());
      s.run();
      assertEquals(SnsrRC.STREAM_END, s.rC());

    } catch (IOException e) {
      fail(e.toString());
    }

    s.release();
  }


  // Returns SnsrStream concatenation of test files.
  private SnsrStream testAudioStream() {
    SnsrStream c = SnsrStream.fromString("");
    for (String tag: TestUsers) {
      for (int i = 4; i <= 5; i++) {
          final String path =
            String.format("%s/%s-%d.wav", BuildConfig.ENROLLMENT_DIR, tag, i);
          c = SnsrStream.fromStreams(c, SnsrStream.fromAudioFile(path, "r"));
      }
    }
    return c;
  }


  // Evaluate model created by enrollFromContextRemoveOne()
  // Just jackalope-1 should spot, as armadillo-1 was removed.
  @Test
  public void evalModelOneUser() {
    SnsrSession s = new SnsrSession();
    final List<String> result = new ArrayList<String>();
    try {
      s .load(SvModel1)
        .require(Snsr.TASK_TYPE, Snsr.PHRASESPOT)
        .setHandler(Snsr.RESULT_EVENT, (session, key) -> {
            result.add(session.getString(Snsr.RES_TEXT));
            return SnsrRC.OK;
          })
        .setStream(Snsr.SOURCE_AUDIO_PCM, testAudioStream())
        .run()
        .release();
    } catch (IOException e) {
      fail(e.toString());
    }
    assertEquals("[jackalope-1, jackalope-1]",
                 result.toString());
  }


  // Evaluate model created by EnrollFromContext().
  // Both armadillo-1 and jackalope-1 should spot.
  @Test
  public void evalModelTwoUsers() {
    SnsrSession s = new SnsrSession();
    final List<String> result = new ArrayList<String>();
    try {
      s .load(SvModel2)
        .require(Snsr.TASK_TYPE, Snsr.PHRASESPOT)
        .setHandler(Snsr.RESULT_EVENT, (session, key) -> {
            result.add(session.getString(Snsr.RES_TEXT));
            return SnsrRC.OK;
          })
        .setStream(Snsr.SOURCE_AUDIO_PCM, testAudioStream())
        .run()
        .release();
    } catch (IOException e) {
      fail(e.toString());
    }
    assertEquals("[armadillo-1, armadillo-1, jackalope-1, jackalope-1]",
                 result.toString());
  }

}
