//
//  ColorOutAnimation.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Cocoa

class ColorOutAnimation: AnimationBase {
    
    let steps = 40
    let rampSteps = 20
    var colors: [(Double, Double, Double)] = []
    var index = 0
    var increment = 1
    var colorIndex = 0

    init() {
        super.init(name: "Color Out")
        
        for hue in stride(from: 0.0, to: 1.0, by: 0.1) {
            let color = NSColor(hue: CGFloat(hue), saturation: 1.0, brightness: 1.0, alpha: 1.0)
            let r = Double(color.redComponent)
            let g = Double(color.greenComponent)
            let b = Double(color.blueComponent)
            colors.append((r, g, b))
        }

        repeat {
            calculateStep()
        } while !((index == 0) && (colorIndex == 0))
        
        export()
    }

    func calculateStep() {
        let (r, g, b) = colors[colorIndex]
        for i in 0 ..< 13 {
            let l = assembly.pixelLocations[i]
            let d = sqrt(l.x * l.x + l.y * l.y)
            let f = d / assembly.maxDistance
            let s: Double
            let c: Double
            if index < rampSteps {
                c = 0
                s = Double(index) / Double(rampSteps + 1)
            } else {
                c = Double(max(index - rampSteps, 0)) / Double(steps - rampSteps)
                s = 1
            }
            let a = s * (1 - abs(f - c))
            let red = a * r
            let green = a * g
            let blue = a * b
            append(red: red, green: green, blue: blue)
        }
        
        calculateAdvance()
    }

    func calculateAdvance() {
        index += increment
        if index >= (steps - 1) {
            increment = -1
        }
        if index <= 0 {
            increment = +1
        }
        if index == 0 {
            colorIndex = (colorIndex + 1) % colors.count
        }
    }
    
    override func exportBody() {
        for i in 0 ..< colors.count {
            emit("    fd_snowflake_operation_illuminate, fd_snowflake_operand(78), fd_snowflake_operand(78 * 13 * \(i)),")
            emit("    fd_snowflake_operation_sleep, fd_snowflake_operand(5),")
        }
    }
    
}
