//
//  ColorOutAnimation.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Cocoa

class ColorOutAnimation: Animation {

    var name = "Color Out"
    let steps = 40
    let rampSteps = 20
    var index = 0
    var increment = 1
    var pixels = [
        Pixel(x:   0.000, y:  15.748),
        Pixel(x:  -7.620, y:   7.620),
        Pixel(x:   0.000, y:   7.874),
        Pixel(x:   7.620, y:   7.620),
        Pixel(x: -15.748, y:   0.000),
        Pixel(x:  -7.874, y:   0.000),
        Pixel(x:   0.000, y:   0.000),
        Pixel(x:   7.874, y:   0.000),
        Pixel(x:  15.748, y:   0.000),
        Pixel(x:  -7.620, y:  -7.620),
        Pixel(x:   0.000, y:  -7.874),
        Pixel(x:   7.620, y:  -7.620),
        Pixel(x:   0.000, y: -15.748),
    ]
    let maxDistance: Double
    var colorIndex = 0
    var colors: [(Double, Double, Double)] = []

    init() {
        var maxDistance: Double = 0
        for pixel in pixels {
            let x = pixel.x
            let y = pixel.y
            let d = sqrt(x * x + y * y)
            if d > maxDistance {
                maxDistance = d
            }
        }
        self.maxDistance = maxDistance
        for hue in stride(from: 0.0, to: 1.0, by: 0.1) {
            let color = NSColor(hue: CGFloat(hue), saturation: 1.0, brightness: 1.0, alpha: 1.0)
            let r = Double(color.redComponent)
            let g = Double(color.greenComponent)
            let b = Double(color.blueComponent)
            colors.append((r, g, b))
        }
    }

    func initialize() {
        index = 0
        colorIndex = 0
    }

    func step() {
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

        let (r, g, b) = colors[colorIndex]
        for i in 0 ..< 13 {
            let p = pixels[i]
            let d = sqrt(p.x * p.x + p.y * p.y)
            let f = d / maxDistance
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
            p.red = a * r
            p.green = a * g
            p.blue = a * b
        }
    }

    func pixel(index: Int) -> Pixel {
        return pixels[index]
    }

}
