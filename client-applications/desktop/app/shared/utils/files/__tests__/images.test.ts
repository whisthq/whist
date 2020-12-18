import { SVGConverter } from "shared/utils/files/images"

/* Define test variables here */
const SVG_IMAGE =
    "https://fractal-supported-app-images.s3.amazonaws.com/chrome-256.svg"

const SVG_NAME = "Google Chrome"

/* Begin tests */
describe("images.ts", () => {
    test("convertToPngBase64(): Convert SVG to base64 encoded PNG", async () => {
        const base64 = await SVGConverter.convertToPngBase64(SVG_IMAGE)
        expect(base64).toContain("data:image/png;base64")
    })
    test("convertToIco(): Convert SVG to Ico", async () => {
        const buffer = await SVGConverter.convertToIco(SVG_IMAGE)
        expect(buffer).not.toBeNull()
        if (buffer) {
            expect(Buffer.from(buffer).toString("base64")).toBeTruthy()
        }
    })
    test("convertToIcns(): Convert SVG to Icns", async () => {
        const buffer = await SVGConverter.convertToIcns(SVG_IMAGE)
        expect(buffer).not.toBeNull()
        if (buffer) {
            expect(Buffer.from(buffer).toString("base64")).toBeTruthy()
        }
    })
    test("saveAsPngTemp(): Save SVG as temporary PNG ", async () => {
        const pngPath = await SVGConverter.saveAsPngTemp(SVG_IMAGE, SVG_NAME)
        expect(pngPath).toContain(`${SVG_NAME}.png`)
    })
})
