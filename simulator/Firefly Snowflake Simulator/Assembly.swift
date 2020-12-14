//
//  Assembly.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/7/20.
//  Copyright © 2020 Firefly Design LLC. All rights reserved.
//

import Foundation

class Assembly {
    
    struct PixelLocation {
        let name: String
        let x: Double
        let y: Double
    }
    
    var pixelLocations = [
        PixelLocation(name:  "U1", x:   0.000, y:  15.748),
        PixelLocation(name:  "U2", x:  -7.620, y:   7.620),
        PixelLocation(name:  "U3", x:   0.000, y:   7.874),
        PixelLocation(name:  "U4", x:   7.620, y:   7.620),
        PixelLocation(name:  "U5", x:  15.748, y:   0.000),
        PixelLocation(name:  "U6", x:   7.874, y:   0.000),
        PixelLocation(name:  "U7", x:   0.000, y:   0.000),
        PixelLocation(name:  "U8", x:  -7.874, y:   0.000),
        PixelLocation(name:  "U9", x: -15.748, y:   0.000),
        PixelLocation(name: "U10", x:  -7.620, y:  -7.620),
        PixelLocation(name: "U11", x:   0.000, y:  -7.874),
        PixelLocation(name: "U12", x:   7.620, y:  -7.620),
        PixelLocation(name: "U13", x:   0.000, y: -15.748),
    ]
    let maxDistance: Double

    init() {
        var maxDistance: Double = 0
        for pixelLocation in pixelLocations {
            let x = pixelLocation.x
            let y = pixelLocation.y
            let d = sqrt(x * x + y * y)
            if d > maxDistance {
                maxDistance = d
            }
        }
        self.maxDistance = maxDistance
    }
    
}
