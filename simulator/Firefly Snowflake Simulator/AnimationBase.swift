//
//  AnimationBase.swift
//  Firefly Snowflake Simulator
//
//  Created by Denis Bohm on 12/7/20.
//  Copyright © 2020 Firefly Design LLC. All rights reserved.
//

import Foundation

class AnimationBase: Animation {
    
    let name: String
    let intensity = 0.25
    let assembly = Assembly()
    var grbzs: [UInt32] = []
    var grbzsIndex = 0
    var firmware = ""
    var stepPixels = [Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel(), Pixel()]

    init(name: String) {
        self.name = name
    }
    
    func initialize() {
    }
    
    func append(red: Double, green: Double, blue: Double) {
        let rByte = UInt32(round(red * 255.0 * intensity))
        let gByte = UInt32(round(green * 255.0 * intensity))
        let bByte = UInt32(round(blue * 255.0 * intensity))
        let grbz = (gByte << 24) | (rByte << 16) | (bByte << 8)
        grbzs.append(grbz)
    }

    func step() {
        for i in 0 ..< 13 {
            let grbz = grbzs[grbzsIndex]
            grbzsIndex = (grbzsIndex + 1) % grbzs.count
            let g = Double((grbz >> 24) & 0xff) / 255.0
            let r = Double((grbz >> 16) & 0xff) / 255.0
            let b = Double((grbz >> 8) & 0xff) / 255.0
            let p = stepPixels[i]
            p.red = r
            p.green = g
            p.blue = b
        }
    }

    func pixel(index: Int) -> Pixel {
        return stepPixels[index]
    }

    func emit(_ line: String = "", terminator: String = "\n") {
        firmware += line
        firmware += terminator
    }
    
    func exportHead() {
        emit("#include \"fd_breathe_animation.h\"")
        emit()
        emit("const uint32_t fd_breathe_grbzs[] = {")
        var i = 0
        for grbz in grbzs {
            emit(String(format: " 0x%08x,", grbz), terminator: "")
            i += 1
            if i >= 13 {
                emit()
                i = 0
            }
        }
        emit("};")
        emit()
        emit("const uint8_t fd_breathe_instructions[] = {")
    }
    
    func exportTail() {
        emit("    fd_snowflake_operation_restart,")
        emit("};")
        emit()
        emit("const fd_snowflake_program_t fd_breathe_program = {")
        emit("    .instructions = fd_breathe_instructions,")
        emit("    .grbzs = fd_breathe_grbzs,")
        emit("};")
    }
    
    func exportBody() {
    }
    
    func export() {
        exportHead()
        exportBody()
        exportTail()
    }

}
