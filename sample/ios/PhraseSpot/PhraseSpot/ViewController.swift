//
//  ViewController.swift
//  PhraseSpot
//
//  Copyright © 2018-2025 Sensory, Inc. https://sensory.com/
//  All rights reserved.
//

import UIKit

class ViewController: UIViewController, PhraseSpotDelegate {
    //MARK: Properties
    @IBOutlet weak var modelName: UILabel!
    @IBOutlet weak var consoleOutput: UITextView!
    @IBOutlet weak var clearButton: UIButton!
    @IBOutlet weak var runButton: UIButton!
    var spotter: PhraseSpot?

    //MARK: Private functions
    private func disableUI() {
        runButton.isEnabled = false
        clearButton.isEnabled = false
    }

    //MARK: Lifecycle
    override func viewDidLoad() {
        super.viewDidLoad()

        // Initialize a PhraseSpot instance from a model in the app bundle.
        do {
            try spotter = PhraseSpot(modelName: "spot-hbg-enUS-1.4.0-m")
            spotter?.delegate = self
            PhraseSpotModel.currentSpotter = spotter
            if let headerMessage = spotter?.libraryInfo {
                logToConsole(headerMessage + "\n\n")
            }

        } catch PhraseSpotError.api(_, let message) {
            logToConsole("Could not load model: " + message)
            disableUI()

        } catch {
            logToConsole("Unexpected error.")
            disableUI()
        }
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        spotter?.start()
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        spotter?.stop()
    }

    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
    }

    //MARK: SpotterModelDelegate
    func recogniserWillStart() {
        let msg = "Listening for \"hello blue genie\"..."
        logToConsole(msg + "\n")
        print(msg)
    }

    func recognizerDidStop(code: SnsrRC, message: String) {
        logToConsole("Stopped listening.\n")
        print("Stopped listening [\(code) \(message)]")
    }

    func recognizerDidSpot(text: String, beginMs: Double, endMs: Double) {
        let msg = String(format: "Spotted \"%@\" from %.3f s to %.3f s.",
                         text, beginMs / 1000, endMs / 1000)
        logToConsole(msg + "\n")
        print(msg)
    }

    //MARK: Utilities
    // Append a string to the log window, then scroll to the end.
    func logToConsole(_ text: String) {
        let eod = consoleOutput.endOfDocument
        consoleOutput.selectedTextRange = consoleOutput.textRange(from: eod, to: eod)
        consoleOutput.insertText(text)
        let length = consoleOutput.text.count
        consoleOutput.scrollRangeToVisible(NSMakeRange(length - 1, 0))
    }

    //MARK: Actions
    @IBAction func clearConsole(_ sender: Any) {
        consoleOutput.text = nil
    }

    @IBAction func runButtonPressed(_ sender: Any) {
        switch spotter!.state {
        case .started:
            spotter?.stop()
            runButton.setTitle("Start", for: .normal)
        default:
            spotter?.start()
            runButton.setTitle("Stop", for: .normal)
        }
    }
}
