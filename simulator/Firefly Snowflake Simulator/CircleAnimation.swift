//
//  CircleAnimation.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/20.
//  Copyright © 2020 Firefly Design LLC. All rights reserved.
//

import Cocoa

class CircleAnimation: AnimationBase {
    
    var colors: [(Double, Double, Double)] = []
    var colorIndex = 0
    let steps = 40
    var stepIndex = 0

    init() {
        super.init(name: "Circle")
        for hue in stride(from: 0.0, to: 1.0, by: 0.1) {
            let color = NSColor(hue: CGFloat(hue), saturation: 1.0, brightness: 1.0, alpha: 1.0)
            let r = Double(color.redComponent)
            let g = Double(color.greenComponent)
            let b = Double(color.blueComponent)
            colors.append((r, g, b))
        }

        repeat {
            calculateStep()
        } while !((stepIndex == 0) && (colorIndex == 0))
        
        export()
    }

    func calculateStep() {
        let (r, g, b) = colors[colorIndex]
        for i in 0 ..< 13 {
            let angle = 2 * Double.pi * Double(stepIndex) / Double(steps)
            let x = assembly.maxDistance * cos(angle)
            let y = assembly.maxDistance * sin(angle)
            let l = assembly.pixelLocations[i]
            let dx = l.x - x
            let dy = l.y - y
            let d = sqrt(dx * dx + dy * dy)
            let a = (stepIndex == steps - 1) ? 0 : max(1 - d / assembly.maxDistance, 0)
            let red = a * r
            let green = a * g
            let blue = a * b
            append(red: red, green: green, blue: blue)
        }
        
        calculateAdvance()
    }

    func calculateAdvance() {
        stepIndex = (stepIndex + 1) % steps
        if stepIndex == 0 {
            colorIndex = (colorIndex + 1) % colors.count
        }
    }
    
    override func exportBody() {
        for i in 0 ..< colors.count {
            emit("    fd_snowflake_operation_illuminate, fd_snowflake_operand(40), fd_snowflake_operand(40 * 13 * \(i)),")
            emit("    fd_snowflake_operation_sleep, fd_snowflake_operand(58),")
        }
    }

}
