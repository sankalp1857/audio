/*
@ @licstart  The following is the entire license notice for the
JavaScript code in this file.

Copyright (C) 1997-2017 by Dimitri van Heesch

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

@licend  The above is the entire license notice
for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "C Docs (7.4.0)", "index.html", [
    [ "Overview", "index.html", null ],
    [ "Changelog", "changelog.html", null ],
    [ "Conceptual Model", "concept-model.html", null ],
    [ "Quick Start", "quick-start.html", [
      [ "C sample code", "quick-start.html#quick-start-c", null ],
      [ "iOS sample apps", "quick-start.html#quick-start-ios", null ]
    ] ],
    [ "Command-line Tools", "tools.html", [
      [ "audio-check", "tools.html#audio-check", null ],
      [ "live-enroll", "tools.html#live-enroll", null ],
      [ "snsr-edit", "tools.html#snsr-edit", null ],
      [ "snsr-eval", "tools.html#snsr-eval", null ],
      [ "snsr-log-split", "tools.html#snsr-log-split", null ],
      [ "spot-convert", "tools.html#spot-convert", null ],
      [ "spot-enroll", "tools.html#spot-enroll", null ],
      [ "spot-eval-batch", "tools.html#spot-eval-batch", null ]
    ] ],
    [ "Custom Recognition", "build.html", [
      [ "Why use a custom recognizer?", "build.html#build-motivation", null ],
      [ "Creating a recognizer with the command-line tools", "build.html#build-cmdline", [
        [ "Class-based recognition", "build.html#build-class", [
          [ "Binary class libraries", "build.html#class-lib", null ]
        ] ]
      ] ],
      [ "Creating a recognizer using the API", "build.html#build-code", [
        [ "Class-based recognition using the API", "build.html#build-code-class", null ]
      ] ],
      [ "Grammar Specification Syntax", "build.html#grammar-syntax", [
        [ "Overview", "build.html#grammar-syntax-overview", null ],
        [ "Operator precedence", "build.html#grammar-op-prec", null ],
        [ "Special symbols", "build.html#grammar-symbols", null ]
      ] ]
    ] ],
    [ "Frequently Asked Questions", "faq.html", [
      [ "Is this SDK thread-safe?", "faq.html#thread-safe", null ],
      [ "How do I debug a phrase spotter?", "faq.html#debug-phrase-spotters", null ],
      [ "How do I run Enrolled Fixed Trigger models?", "faq.html#use-enrolled-triggers", null ],
      [ "Can I run two phrase spotters at the same time?", "faq.html#run-two-spotters", null ],
      [ "What is a Command Set?", "faq.html#use-command-set", null ],
      [ "Can I create a trigger-to-search model?", "faq.html#trigger-to-search", null ],
      [ "How do I improve the user experience in marginal conditions?", "faq.html#improve-user-experience", null ],
      [ "Can I use models from the beta releases?", "faq.html#beta-model-compatibility", null ],
      [ "How can I reduce application code size?", "faq.html#reduce-code-size", null ],
      [ "How do I spot phrases on a Real-Time Operating System (RTOS) with a custom audio driver and no filesystem?", "faq.html#program-for-rtos", null ],
      [ "Can I avoid dynamic memory allocation?", "faq.html#custom-heap-allocator", null ],
      [ "How do I improve spotter performance?", "faq.html#improve-performance", [
        [ "How to measure real-time factor and MIPS", "faq.html#measure-real-time", null ],
        [ "What if the spotter runs too slow, or consumes too many cycles?", "faq.html#slower-than-rt", null ],
        [ "What if the spotter consumes too much memory?", "faq.html#low-ram", null ],
        [ "What is a little-big spotter?", "faq.html#little-big", null ],
        [ "What is a frame-stacked spotter?", "faq.html#frame-stacked", null ],
        [ "What is a multi-threaded spotter?", "faq.html#multi-threaded", null ],
        [ "Factors to consider", "faq.html#Key", null ]
      ] ],
      [ "How do I use Large Vocabulary Continuous Speech Recognition?", "faq.html#use-lvcsr", [
        [ "LVCSR without audio segmentation", "faq.html#lvcsr-no-vad", null ],
        [ "LVCSR with lightweight NLU parsing", "faq.html#lvcsr-nlu", [
          [ "NLU with custom grammar recognizers", "faq.html#lvcsr-nlu-grammar", null ],
          [ "NLU with broad-domain recognizers", "faq.html#lvcsr-nlu-bdlm", null ],
          [ "Dealing with NLU parse ambiguity", "faq.html#lvcsr-nlu-nbest", null ],
          [ "How do I take action on an NLU result?", "faq.html#lvcsr-nlu-action", null ]
        ] ],
        [ "LVCSR with VAD-segmented audio", "faq.html#lvcsr-vad", null ],
        [ "LVCSR following a wake word", "faq.html#lvcsr-spot-vad", null ]
      ] ]
    ] ],
    [ "Known Issues", "known-issues.html", null ],
    [ "Contact Information", "contact.html", null ],
    [ "Sensory License Agreement", "license.html", null ],
    [ "Open Source Licenses", "oss-licenses.html", null ],
    [ "Task Descriptions", "task.html", [
      [ "Phrase Spotter", "task.html#task-phrasespot", null ],
      [ "Spotter Enrollment", "task.html#task-enroll", null ],
      [ "Voice Activity Detector", "task.html#task-vad", null ],
      [ "Phrase Spotter with VAD", "task.html#task-phrasespot-vad", null ],
      [ "Large Vocabulary Continuous Speech Recognizer", "task.html#task-lvcsr", null ],
      [ "Task Templates", "task.html#task-templates", [
        [ "tpl-spot-concurrent", "task.html#tpl-spot-concurrent", null ],
        [ "tpl-spot-debug", "task.html#tpl-spot-debug", null ],
        [ "tpl-spot-select", "task.html#tpl-spot-select", null ],
        [ "tpl-spot-sequential", "task.html#tpl-spot-sequential", null ],
        [ "tpl-spot-vad", "task.html#tpl-spot-vad", null ],
        [ "tpl-spot-vad-lvcsr", "task.html#tpl-spot-vad-lvcsr", null ],
        [ "tpl-vad-lvcsr", "task.html#tpl-vad-lvcsr", null ]
      ] ]
    ] ],
    [ "Task Models", "models.html", [
      [ "Phrase Spotters", "models.html#model-phrasespot", null ],
      [ "Spotter Enrollment", "models.html#model-enroll", null ],
      [ "Voice Activity Detector", "models.html#model-vad", null ],
      [ "‡ Large Vocabulary Continuous Speech Recognizer", "models.html#model-lvcsr", null ],
      [ "Templates", "models.html#model-template", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Reference", "modules.html", "modules" ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Globals", "globals.html", [
      [ "All", "globals.html", "globals_dup" ],
      [ "Functions", "globals_func.html", null ],
      [ "Typedefs", "globals_type.html", null ],
      [ "Enumerations", "globals_enum.html", null ],
      [ "Enumerator", "globals_eval.html", null ],
      [ "Macros", "globals_defs.html", null ]
    ] ],
    [ "File List", "files.html", "files" ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"alsa-stream_8c-example.html",
"group__session.html#gaf4f7dab60ba977382ecd78992f27f0c1",
"group__stream-provider.html#gga1b1bc34f0494e7a4c9cc554b3338bb89a6abbc21f4e64d0df2aa2c5060e1aec45",
"group__task-spot.html#gad0dd46e3ec16075db2d478559933677f"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';