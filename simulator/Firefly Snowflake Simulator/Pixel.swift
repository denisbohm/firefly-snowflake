//
//  Pixel.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Cocoa

class Pixel {

    var red: Double
    var green: Double
    var blue: Double
    
    init() {
        self.red = 0
        self.green = 0
        self.blue = 0
    }
    
    init(red: Double, green: Double, blue: Double) {
        self.red = red
        self.green = green
        self.blue = blue
    }

}
