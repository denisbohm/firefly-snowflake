//
//  ViewController.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {

    enum LocalError: Error {
        case subviewNotFound
    }

    @IBOutlet var patternPopUpButton: NSPopUpButton?
    var leds = Array<RGBLEDView>()
    var animations = Array<Animation>()
    var animation: Animation? = nil
    var timer: Timer? = nil

    override func viewDidLoad() {
        super.viewDidLoad()

        do {
            for i in 1 ... 13 {
                leds.append(try subview(toolTip: "U\(i)") as! RGBLEDView)
            }
        } catch {
            NSLog("cannot find subview")
        }

        animations.append(ColorOutAnimation())

        for animation in animations {
            patternPopUpButton?.addItem(withTitle: animation.name)
        }
        animation = animations[0]
        updateLEDS()

        timer = Timer.scheduledTimer(timeInterval: 0.05, target: self, selector: #selector(animate), userInfo: nil, repeats: true)
    }

    func subview(toolTip: String) throws -> NSView {
        for subview in self.view.subviews {
            if let subviewToolTip = subview.toolTip {
                if subviewToolTip == toolTip {
                    return subview
                }
            }
        }
        throw LocalError.subviewNotFound
    }

    func updateLEDS() {
        if let animation = self.animation {
            for i in 0 ..< 13 {
                let pixel = animation.pixel(index: i)
                let red = CGFloat(pixel.red)
                let green = CGFloat(pixel.green)
                let blue = CGFloat(pixel.blue)
                let color = NSColor(red: red, green: green, blue: blue, alpha: 1.0)
                leds[i].color = color
            }
        }
    }

    @objc func animate() {
        animation?.step()
        updateLEDS()
    }

    @IBAction func patternChanged(_: AnyObject) {

    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }

}

