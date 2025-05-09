//
//  PhraseSpot.swift
//  PhraseSpot
//
//  Copyright © 2018-2025 Sensory, Inc. https://sensory.com/
//  All rights reserved.
//

import Foundation
import AVFoundation

protocol PhraseSpotDelegate: AnyObject {
    func recogniserWillStart()
    func recognizerDidStop(code: SnsrRC, message: String)
    func recognizerDidSpot(text: String, beginMs: Double, endMs: Double)
}

enum PhraseSpotError: Error {
    case api(code: SnsrRC, message: String)
}

class PhraseSpot {
    //MARK: Nested classes
    enum State {
        case stopped, started, paused
    }

    //MARK: Private properties
    private var session: SnsrSession?
    private var audio: SnsrStream?
    private var rC: SnsrRC = SNSR_RC_OK


    //MARK: Properties
    var libraryInfo: String?
    weak var delegate: PhraseSpotDelegate?
    var state: State = .stopped {
        didSet {
            if state == .started {
                startRecog()
            }
        }
    }

    //MARK: Initialization
    init(modelName: String) throws {
        try load(modelName: modelName)
    }

    deinit {
        if state == .started {
            stop()
        }
        release(&session)
        release(&audio)
    }

    //MARK: Private methods
    // C library wrappers, for convenience
    private func release(_ ptr: inout Optional<OpaquePointer>) {
        snsrRelease(UnsafeRawPointer(ptr))
        ptr = nil
    }

    private func retain(_ ptr: Optional<OpaquePointer>) {
        snsrRetain(UnsafeRawPointer(ptr))
    }

    // Find a model in the applications main bundle.
    private func modelPath(_ modelName: String) -> String {
        guard let path = Bundle.main.path(forResource: modelName, ofType: "snsr", inDirectory: "models") else {
            return modelName
        }
        return path
    }

    // Create and throw a PhraseSpotError.api
    private func throwIfError(_ session: SnsrSession?) throws {
        let rc = snsrRC(session)
        if (rc != SNSR_RC_OK) {
            let msg = String(cString: snsrErrorDetail(session))
            print(msg)
            throw PhraseSpotError.api(code: rc, message: msg)
        }
    }

    private func load(modelName: String) throws {
        snsrNewIncludeOSS(&session, SNSR_VERSION)
        try throwIfError(session)
        var libInfo: UnsafePointer<CChar>?
        snsrGetString(session, SNSR_LIBRARY_INFO, &libInfo)
        try throwIfError(session)
        libraryInfo = String(cString: libInfo!)
        snsrLoad(session, snsrStreamFromFileName(modelPath(modelName), "r"))
        snsrRequire(session, SNSR_TASK_TYPE, SNSR_PHRASESPOT)
        snsrRequire(session, SNSR_TASK_VERSION, "~0.5.0 || 1.0.0")

        // Convert self into a pointer to pass to the C library
        let selfPtr = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())

        // Report recognition results with a delegate.recognizerDidSpot()
        snsrSetHandler(session, SNSR_RESULT_EVENT,
                       snsrCallback({ (session, key, selfPtr) -> SnsrRC in
                        let my = Unmanaged<PhraseSpot>.fromOpaque(selfPtr!).takeUnretainedValue()
                        var text: UnsafePointer<CChar>?
                        var beginMs: Double = 0
                        var endMs: Double = 0
                        snsrGetString(session, SNSR_RES_TEXT, &text)
                        snsrGetDouble(session, SNSR_RES_BEGIN_MS, &beginMs)
                        snsrGetDouble(session, SNSR_RES_END_MS, &endMs)
                        let spot = String(cString: text!)
                        DispatchQueue.main.sync {
                            my.delegate?.recognizerDidSpot(text: spot, beginMs: beginMs, endMs: endMs)
                        }
                        return SNSR_RC_OK
                       }, nil, selfPtr))

        // Stop background recognition if the state changes from .started
        snsrSetHandler(session, SNSR_SAMPLES_EVENT,
                       snsrCallback({ (session, key, selfPtr) -> SnsrRC in
                        let my = Unmanaged<PhraseSpot>.fromOpaque(selfPtr!).takeUnretainedValue()
                        return my.state == .started ? SNSR_RC_OK : SNSR_RC_STOP
                       }, nil, selfPtr))

        // Allow Bluetooth headsets
        try AVAudioSession.sharedInstance()
            .setCategory(.playAndRecord, options: AVAudioSession.CategoryOptions.allowBluetooth)

        // Live audio
        audio = snsrStreamFromDefaultAudioDevice()
        retain(audio)
        snsrSetStream(session, SNSR_SOURCE_AUDIO_PCM, audio)
        try throwIfError(session)
    }

    // Run phrase spotter on a background thread
    private func startRecog() {
        snsrClearRC(session)
        self.delegate?.recogniserWillStart()
        DispatchQueue.global(qos: .background).async {
            let code = snsrRun(self.session)
            // Stop recording when we are not spotting
            snsrStreamClose(self.audio)
            let msg = String(cString: snsrErrorDetail(self.session))
            DispatchQueue.main.sync {
                self.delegate?.recognizerDidStop(code: code, message: msg)
            }
        }
    }


    //MARK: Public methods
    // Change scalar SnsrSession settings
    func set(_ key: String, _ value: Double) throws {
        let code = snsrSetInt(session, key, Int32(value))
        if code == SNSR_RC_INCORRECT_SETTING_TYPE {
            snsrClearRC(session)
            snsrSetDouble(session, key, value)
        }
        try throwIfError(session)
    }

    // Start the phrase spotter
    func start() {
        if state != .started {
            state = .started
        }
    }

    // Stop the recognizer
    func stop() {
        if state != .stopped {
            state = .stopped
        }
    }

    // Pause a running spotter
    func pause() {
        if state == .started {
            state = .paused
        }
    }

    // Resume a paused spotter
    func resume() {
        if state == .paused {
            state = .started
        }
    }
}
