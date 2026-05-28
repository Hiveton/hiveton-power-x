import CoreGraphics
import CoreText
import Foundation

struct Glyph {
    let scalar: UInt32
    let width: Int
    let height: Int
    let advance: Int
    let data: [UInt8]
}

func alphaAtPacked(_ data: [UInt8], width: Int, x: Int, y: Int) -> UInt8 {
    let byteWidth = (width + 1) / 2
    let packed = data[y * byteWidth + x / 2]
    return (x & 1) == 0 ? (packed >> 4) : (packed & 0x0F)
}

func setAlphaPacked(_ data: inout [UInt8], width: Int, x: Int, y: Int, alpha: UInt8) {
    let byteWidth = (width + 1) / 2
    let index = y * byteWidth + x / 2
    if (x & 1) == 0 {
        data[index] = (data[index] & 0x0F) | (alpha << 4)
    } else {
        data[index] = (data[index] & 0xF0) | (alpha & 0x0F)
    }
}

func normalizeTop(_ glyph: Glyph, threshold: UInt8 = 3) -> Glyph {
    var firstRow = glyph.height

    for y in 0..<glyph.height {
        for x in 0..<glyph.width {
            if alphaAtPacked(glyph.data, width: glyph.width, x: x, y: y) > threshold {
                firstRow = y
                break
            }
        }
        if firstRow != glyph.height {
            break
        }
    }

    if firstRow == 0 || firstRow == glyph.height {
        return glyph
    }

    var shifted = [UInt8](repeating: 0, count: glyph.data.count)
    for y in firstRow..<glyph.height {
        for x in 0..<glyph.width {
            let alpha = alphaAtPacked(glyph.data, width: glyph.width, x: x, y: y)
            if alpha != 0 {
                setAlphaPacked(&shifted, width: glyph.width, x: x, y: y - firstRow, alpha: alpha)
            }
        }
    }

    return Glyph(scalar: glyph.scalar,
                 width: glyph.width,
                 height: glyph.height,
                 advance: glyph.advance,
                 data: shifted)
}

func alignBottom(_ glyph: Glyph, targetBottom: Int, threshold: UInt8 = 3) -> Glyph {
    var bottomRow = -1

    for y in 0..<glyph.height {
        for x in 0..<glyph.width {
            if alphaAtPacked(glyph.data, width: glyph.width, x: x, y: y) > threshold {
                bottomRow = y
                break
            }
        }
    }

    if bottomRow < 0 || bottomRow == targetBottom {
        return glyph
    }

    let delta = targetBottom - bottomRow
    var shifted = [UInt8](repeating: 0, count: glyph.data.count)
    for y in 0..<glyph.height {
        let targetY = y + delta
        if targetY < 0 || targetY >= glyph.height {
            continue
        }
        for x in 0..<glyph.width {
            let alpha = alphaAtPacked(glyph.data, width: glyph.width, x: x, y: y)
            if alpha != 0 {
                setAlphaPacked(&shifted, width: glyph.width, x: x, y: targetY, alpha: alpha)
            }
        }
    }

    return Glyph(scalar: glyph.scalar,
                 width: glyph.width,
                 height: glyph.height,
                 advance: glyph.advance,
                 data: shifted)
}

func makeDotGlyph(width: Int, height: Int, advance: Int) -> Glyph {
    var packed = [UInt8]()
    for y in 0..<height {
        var x = 0
        while x < width {
            func alphaAt(_ px: Int, _ py: Int) -> UInt8 {
                if px >= 1 && px <= 4 && py >= height - 6 && py <= height - 3 {
                    return 0x0F
                }
                return 0
            }
            let a0 = alphaAt(x, y)
            let a1: UInt8 = (x + 1 < width) ? alphaAt(x + 1, y) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }
    return Glyph(scalar: 0x002E, width: width, height: height, advance: advance, data: packed)
}

func makeSmallDotGlyph() -> Glyph {
    var packed = [UInt8]()
    let width = 6
    let height = 15
    for y in 0..<height {
        var x = 0
        while x < width {
            func alphaAt(_ px: Int, _ py: Int) -> UInt8 {
                if px >= 2 && px <= 4 && py >= 11 && py <= 13 {
                    return 0x0F
                }
                return 0
            }
            let a0 = alphaAt(x, y)
            let a1: UInt8 = (x + 1 < width) ? alphaAt(x + 1, y) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }
    return Glyph(scalar: 0x002E, width: width, height: height, advance: 6, data: packed)
}

func makeTinyDotGlyph() -> Glyph {
    var packed = [UInt8]()
    let width = 5
    let height = 12
    for y in 0..<height {
        var x = 0
        while x < width {
            func alphaAt(_ px: Int, _ py: Int) -> UInt8 {
                if px >= 2 && px <= 3 && py >= 9 && py <= 10 {
                    return 0x0F
                }
                return 0
            }
            let a0 = alphaAt(x, y)
            let a1: UInt8 = (x + 1 < width) ? alphaAt(x + 1, y) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }
    return Glyph(scalar: 0x002E, width: width, height: height, advance: 5, data: packed)
}

func makeFont(name: String, size: CGFloat) -> CTFont {
    if let font = CTFontCreateWithName(name as CFString, size, nil) as CTFont? {
        return font
    }
    return CTFontCreateWithName("Menlo" as CFString, size, nil)
}

func makeFont(filePath: String, size: CGFloat) -> CTFont {
    let url = URL(fileURLWithPath: filePath)
    if let provider = CGDataProvider(url: url as CFURL),
       let cgFont = CGFont(provider) {
        return CTFontCreateWithGraphicsFont(cgFont, size, nil, nil)
    }
    return makeFont(name: "CourierNewPS-BoldMT", size: size)
}

func renderGlyph(font: CTFont, scalar: UInt32, canvasWidth: Int, canvasHeight: Int, baseline: CGFloat, xOffset: CGFloat) -> Glyph {
    let colorSpace = CGColorSpaceCreateDeviceGray()
    var pixels = [UInt8](repeating: 0, count: canvasWidth * canvasHeight)
    let context = CGContext(data: &pixels,
                            width: canvasWidth,
                            height: canvasHeight,
                            bitsPerComponent: 8,
                            bytesPerRow: canvasWidth,
                            space: colorSpace,
                            bitmapInfo: CGImageAlphaInfo.none.rawValue)!
    context.setFillColor(gray: 0, alpha: 1)
    context.fill(CGRect(x: 0, y: 0, width: canvasWidth, height: canvasHeight))
    context.setFillColor(gray: 1, alpha: 1)
    context.setAllowsAntialiasing(true)
    context.setShouldAntialias(true)
    context.translateBy(x: 0, y: CGFloat(canvasHeight))
    context.scaleBy(x: 1, y: -1)
    context.textMatrix = .identity

    let string = String(UnicodeScalar(scalar)!)
    let attributes: [CFString: Any] = [
        kCTFontAttributeName: font,
        kCTForegroundColorAttributeName: CGColor(gray: 1, alpha: 1)
    ]
    let attributed = CFAttributedStringCreate(nil, string as CFString, attributes as CFDictionary)!
    let line = CTLineCreateWithAttributedString(attributed)
    context.textPosition = CGPoint(x: xOffset, y: baseline)
    CTLineDraw(line, context)

    var minX = canvasWidth
    var maxX = 0
    var minY = canvasHeight
    var maxY = 0
    for y in 0..<canvasHeight {
        for x in 0..<canvasWidth {
            if pixels[y * canvasWidth + x] > 4 {
                minX = min(minX, x)
                maxX = max(maxX, x)
                minY = min(minY, y)
                maxY = max(maxY, y)
            }
        }
    }

    if minX > maxX || minY > maxY {
        return Glyph(scalar: scalar, width: 3, height: canvasHeight, advance: 4,
                     data: [UInt8](repeating: 0, count: ((3 + 1) / 2) * canvasHeight))
    }

    let width = maxX - minX + 1
    var packed = [UInt8]()
    for y in 0..<canvasHeight {
        let sourceY = canvasHeight - 1 - y
        var x = 0
        while x < width {
            let a0 = pixels[sourceY * canvasWidth + minX + x] >> 4
            let a1: UInt8 = (x + 1 < width) ? (pixels[sourceY * canvasWidth + minX + x + 1] >> 4) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }

    return Glyph(scalar: scalar, width: width, height: canvasHeight, advance: width + 1, data: packed)
}

func renderFixedGlyph(font: CTFont,
                      scalar: UInt32,
                      canvasWidth: Int,
                      canvasHeight: Int,
                      yOffset: CGFloat,
                      xOffset: CGFloat,
                      advance: Int) -> Glyph {
    let colorSpace = CGColorSpaceCreateDeviceGray()
    var pixels = [UInt8](repeating: 0, count: canvasWidth * canvasHeight)
    let context = CGContext(data: &pixels,
                            width: canvasWidth,
                            height: canvasHeight,
                            bitsPerComponent: 8,
                            bytesPerRow: canvasWidth,
                            space: colorSpace,
                            bitmapInfo: CGImageAlphaInfo.none.rawValue)!
    context.setFillColor(gray: 0, alpha: 1)
    context.fill(CGRect(x: 0, y: 0, width: canvasWidth, height: canvasHeight))
    context.setAllowsAntialiasing(true)
    context.setShouldAntialias(true)
    context.translateBy(x: 0, y: CGFloat(canvasHeight))
    context.scaleBy(x: 1, y: -1)
    context.textMatrix = .identity

    let string = String(UnicodeScalar(scalar)!)
    let attributes: [CFString: Any] = [
        kCTFontAttributeName: font,
        kCTForegroundColorAttributeName: CGColor(gray: 1, alpha: 1)
    ]
    let attributed = CFAttributedStringCreate(nil, string as CFString, attributes as CFDictionary)!
    let line = CTLineCreateWithAttributedString(attributed)
    let bounds = CTLineGetBoundsWithOptions(line, [.useGlyphPathBounds])
    let centeredY = (CGFloat(canvasHeight) - bounds.height) / 2.0 + yOffset
    let baseline = centeredY - bounds.minY
    context.textPosition = CGPoint(x: xOffset, y: baseline)
    CTLineDraw(line, context)

    var packed = [UInt8]()
    for y in 0..<canvasHeight {
        let sourceY = canvasHeight - 1 - y
        var x = 0
        while x < canvasWidth {
            let a0 = pixels[sourceY * canvasWidth + x] >> 4
            let a1: UInt8 = (x + 1 < canvasWidth) ? (pixels[sourceY * canvasWidth + x + 1] >> 4) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }

    return Glyph(scalar: scalar, width: canvasWidth, height: canvasHeight, advance: advance, data: packed)
}

func renderFixedBaselineGlyph(font: CTFont,
                              scalar: UInt32,
                              canvasWidth: Int,
                              canvasHeight: Int,
                              baseline: CGFloat,
                              xOffset: CGFloat,
                              advance: Int) -> Glyph {
    let colorSpace = CGColorSpaceCreateDeviceGray()
    var pixels = [UInt8](repeating: 0, count: canvasWidth * canvasHeight)
    let context = CGContext(data: &pixels,
                            width: canvasWidth,
                            height: canvasHeight,
                            bitsPerComponent: 8,
                            bytesPerRow: canvasWidth,
                            space: colorSpace,
                            bitmapInfo: CGImageAlphaInfo.none.rawValue)!
    context.setFillColor(gray: 0, alpha: 1)
    context.fill(CGRect(x: 0, y: 0, width: canvasWidth, height: canvasHeight))
    context.setAllowsAntialiasing(true)
    context.setShouldAntialias(true)
    context.translateBy(x: 0, y: CGFloat(canvasHeight))
    context.scaleBy(x: 1, y: -1)
    context.textMatrix = .identity

    let string = String(UnicodeScalar(scalar)!)
    let attributes: [CFString: Any] = [
        kCTFontAttributeName: font,
        kCTForegroundColorAttributeName: CGColor(gray: 1, alpha: 1)
    ]
    let attributed = CFAttributedStringCreate(nil, string as CFString, attributes as CFDictionary)!
    let line = CTLineCreateWithAttributedString(attributed)
    context.textPosition = CGPoint(x: xOffset, y: baseline)
    CTLineDraw(line, context)

    var packed = [UInt8]()
    for y in 0..<canvasHeight {
        let sourceY = canvasHeight - 1 - y
        var x = 0
        while x < canvasWidth {
            let a0 = pixels[sourceY * canvasWidth + x] >> 4
            let a1: UInt8 = (x + 1 < canvasWidth) ? (pixels[sourceY * canvasWidth + x + 1] >> 4) : 0
            packed.append((a0 << 4) | a1)
            x += 2
        }
    }

    return Glyph(scalar: scalar, width: canvasWidth, height: canvasHeight, advance: advance, data: packed)
}

func cName(_ prefix: String, _ scalar: UInt32) -> String {
    return "\(prefix)_\(String(format: "%04X", scalar))"
}

func emitHeader(path: String, guardName: String, prefix: String, glyphs: [Glyph]) throws {
    var out = ""
    out += "#ifndef \(guardName)\n#define \(guardName)\n\n"
    out += "#include <stdint.h>\n\n"
    out += "typedef struct\n{\n    uint16_t codepoint;\n    uint8_t width;\n    uint8_t height;\n    uint8_t advance;\n    const uint8_t *data;\n} \(prefix)_glyph_t;\n\n"
    for glyph in glyphs {
        out += "static const uint8_t \(cName(prefix, glyph.scalar))[] = {"
        for (index, byte) in glyph.data.enumerated() {
            if index % 16 == 0 { out += "\n    " }
            out += String(format: "0x%02XU", byte)
            out += (index + 1 == glyph.data.count) ? "" : ", "
        }
        out += "\n};\n\n"
    }
    out += "static const \(prefix)_glyph_t g_\(prefix)_glyphs[] =\n{\n"
    for glyph in glyphs {
        out += String(format: "    { 0x%04XU, %uU, %uU, %uU, %@ },\n",
                      glyph.scalar, glyph.width, glyph.height, glyph.advance, cName(prefix, glyph.scalar))
    }
    out += "};\n\n"
    out += "#define \(prefix.uppercased())_GLYPH_COUNT (sizeof(g_\(prefix)_glyphs) / sizeof(g_\(prefix)_glyphs[0]))\n\n"
    out += "#endif /* \(guardName) */\n"
    try out.write(toFile: path, atomically: true, encoding: .utf8)
}

let root = FileManager.default.currentDirectoryPath
let digitFont = makeFont(filePath: "\(root)/PowerXCode/MonospaceTypewriter-1.ttf", size: 29)
let smallMonoFont = makeFont(filePath: "\(root)/PowerXCode/MonospaceTypewriter-1.ttf", size: 17)
let mono12Font = makeFont(filePath: "\(root)/PowerXCode/MonospaceTypewriter-1.ttf", size: 14)
let mono9Font = makeFont(filePath: "\(root)/PowerXCode/MonospaceTypewriter-1.ttf", size: 11)
let zhFont = makeFont(name: "PingFangSC-Regular", size: 23)
let zh15Font = makeFont(name: "HiraginoSansGB-W3", size: 16)

let meterChars: [UInt32] = Array("0123456789.VAW".unicodeScalars.map { $0.value })
let smallMonoChars: [UInt32] = Array("0123456789.%+-VAWDCPQFSAHEIOUTRLMNK<>mAh ".unicodeScalars.map { $0.value }).reduce(into: [UInt32]()) {
    if !$0.contains($1) {
        $0.append($1)
    }
}
let mono12Chars: [UInt32] = Array("0123456789:mAhWH.PQCDSFTEYRLNVOUIA+- ".unicodeScalars.map { $0.value }).reduce(into: [UInt32]()) {
    if !$0.contains($1) {
        $0.append($1)
    }
}
let mono9Chars: [UInt32] = Array("0123456789.:-/ABCDEFGHKMNPQRSTUVWY ".unicodeScalars.map { $0.value }).reduce(into: [UInt32]()) {
    if !$0.contains($1) {
        $0.append($1)
    }
}
let zhChars: [UInt32] = [0x7535, 0x538B, 0x6D41, 0x529F, 0x7387]
let zh15Chars: [UInt32] = [
    0x6700, 0x5927, 0x5E73, 0x5747, 0x7EDF, 0x8BA1, 0x65F6, 0x95F4,
    0x68C0, 0x6D4B, 0x534F, 0x8BAE, 0x8BFB, 0x53D6, 0x6A21, 0x62DF,
    0x5F00, 0x5C14, 0x6587, 0x7EBF, 0x963B, 0x8BBE, 0x7F6E,
    0x7F16, 0x8F91, 0x67E5, 0x770B, 0x4EAE, 0x5EA6, 0x65CB, 0x8F6C,
    0x8BF1, 0x9A97, 0x624B, 0x52A8, 0x81EA, 0x62A5,
    0x79FB, 0x9664, 0x8D1F, 0x8F7D, 0x786E, 0x8BA4, 0x6D88
]

let meterGlyphs = meterChars.map {
    if $0 == 0x002E {
        return makeDotGlyph(width: 6, height: 26, advance: 8)
    }
    if $0 == 0x0056 || $0 == 0x0041 || $0 == 0x0057 {
        return alignBottom(normalizeTop(renderFixedGlyph(font: digitFont, scalar: $0, canvasWidth: 21, canvasHeight: 26, yOffset: 0, xOffset: 0, advance: 20)),
                           targetBottom: 23)
    }
    return alignBottom(normalizeTop(renderFixedGlyph(font: digitFont, scalar: $0, canvasWidth: 15, canvasHeight: 26, yOffset: 0, xOffset: -2, advance: 15)),
                       targetBottom: 23)
}
let zhGlyphs = zhChars.map {
    renderFixedGlyph(font: zhFont, scalar: $0, canvasWidth: 24, canvasHeight: 24, yOffset: 0, xOffset: 0, advance: 24)
}
let smallMonoGlyphs = smallMonoChars.map {
    if $0 == 0x0020 {
        return Glyph(scalar: $0, width: 4, height: 15, advance: 4,
                     data: [UInt8](repeating: 0, count: ((4 + 1) / 2) * 15))
    }
    if $0 == 0x002E {
        return makeSmallDotGlyph()
    }
    if $0 == 0x0056 || $0 == 0x0041 || $0 == 0x0057 {
        return renderFixedGlyph(font: smallMonoFont, scalar: $0, canvasWidth: 10, canvasHeight: 15, yOffset: 0, xOffset: 0, advance: 9)
    }
    return renderFixedGlyph(font: smallMonoFont, scalar: $0, canvasWidth: 9, canvasHeight: 15, yOffset: 0, xOffset: 0, advance: 9)
}
let mono12Glyphs = mono12Chars.map {
    if $0 == 0x0020 {
        return Glyph(scalar: $0, width: 4, height: 12, advance: 4,
                     data: [UInt8](repeating: 0, count: ((4 + 1) / 2) * 12))
    }
    if $0 == 0x002E {
        return makeTinyDotGlyph()
    }
    return renderFixedGlyph(font: mono12Font, scalar: $0, canvasWidth: 7, canvasHeight: 12, yOffset: 0, xOffset: -1, advance: 6)
}
let mono9Glyphs = mono9Chars.map {
    if $0 == 0x0020 {
        return Glyph(scalar: $0, width: 3, height: 9, advance: 3,
                     data: [UInt8](repeating: 0, count: ((3 + 1) / 2) * 9))
    }
    if $0 == 0x002E {
        var packed = [UInt8]()
        let width = 4
        let height = 9
        for y in 0..<height {
            var x = 0
            while x < width {
                func alphaAt(_ px: Int, _ py: Int) -> UInt8 {
                    if px >= 1 && px <= 2 && py >= 7 && py <= 8 {
                        return 0x0F
                    }
                    return 0
                }
                let a0 = alphaAt(x, y)
                let a1: UInt8 = (x + 1 < width) ? alphaAt(x + 1, y) : 0
                packed.append((a0 << 4) | a1)
                x += 2
            }
        }
        return Glyph(scalar: $0, width: width, height: height, advance: 4, data: packed)
    }
    if $0 == 0x0056 || $0 == 0x0041 || $0 == 0x0057 {
        return renderFixedGlyph(font: mono9Font, scalar: $0, canvasWidth: 8, canvasHeight: 9, yOffset: 0, xOffset: 0, advance: 7)
    }
    return renderFixedGlyph(font: mono9Font, scalar: $0, canvasWidth: 7, canvasHeight: 9, yOffset: 0, xOffset: 0, advance: 7)
}
let zh15Glyphs = zh15Chars.map {
    renderFixedGlyph(font: zh15Font, scalar: $0, canvasWidth: 14, canvasHeight: 15, yOffset: 0, xOffset: -1, advance: 13)
}

try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_meter_24.h",
               guardName: "FONT_METER_24_H",
               prefix: "font_meter_24",
               glyphs: meterGlyphs)
try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_mono_15.h",
               guardName: "FONT_MONO_15_H",
               prefix: "font_mono_15",
               glyphs: smallMonoGlyphs)
try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_mono_12.h",
               guardName: "FONT_MONO_12_H",
               prefix: "font_mono_12",
               glyphs: mono12Glyphs)
try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_mono_9.h",
               guardName: "FONT_MONO_9_H",
               prefix: "font_mono_9",
               glyphs: mono9Glyphs)
try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_zh_24.h",
               guardName: "FONT_ZH_24_H",
               prefix: "font_zh_24",
               glyphs: zhGlyphs)
try emitHeader(path: "\(root)/PowerXCode/FreeRTOS/Assets/font_zh_15.h",
               guardName: "FONT_ZH_15_H",
               prefix: "font_zh_15",
               glyphs: zh15Glyphs)
