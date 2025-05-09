//
//  PhraseSpotTests.swift
//  PhraseSpotTests
//
//  Copyright © 2018-2025 Sensory, Inc. https://sensory.com/
//  All rights reserved.
//

import XCTest
@testable import PhraseSpot

class PhraseSpotTests: XCTestCase {
    
    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }

    func testPhraseSpotInitializer() {
        let spotter = try? PhraseSpot(modelName: "spot-hbg-enUS-1.4.0-m")
        XCTAssertNotNil(spotter)
    }

    func testPhraseSpotInitializerModelNotFound() {
        do {
            let _ = try PhraseSpot(modelName: "no-such-model")
        } catch PhraseSpotError.api(let code, let message) {
            print(message)
            print(code)
            XCTAssertEqual(code, SNSR_RC_STREAM)
        } catch {
            XCTFail()
        }
    }

    func testPerformanceExample() {
        // This is an example of a performance test case.
        self.measure {
            // Put the code you want to measure the time of here.
        }
    }
    
}
