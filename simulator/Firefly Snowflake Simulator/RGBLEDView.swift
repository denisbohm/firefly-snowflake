//
//  RGBLEDView.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Cocoa

open class RGBLEDView: NSView {

    open var color: NSColor = NSColor.black {
        didSet {
            self.needsDisplay = true
        }
    }

    override open func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        color.setFill()
        NSBezierPath.fill(NSRect(origin: CGPoint(x: 0, y: 0), size: self.frame.size))
    }
    
}
