//
//  Animation.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/8/16.
//  Copyright © 2016 Firefly Design LLC. All rights reserved.
//

import Foundation

protocol Animation {

    var name: String { get }
    
    func initialize()

    func step()

    func pixel(index: Int) -> Pixel

}
