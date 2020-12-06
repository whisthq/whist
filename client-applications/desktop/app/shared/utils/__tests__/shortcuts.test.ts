/* eslint-disable */

import { SVGConverter } from "shared/utils/shortcuts"

// SVGConverter() variables
let slackSVG =
    "https://fractal-supported-app-images.s3.amazonaws.com/slack-256.svg"

/* Begin tests */
describe("shortcuts.ts", () => {
    test("SVGConverter(): Convert valid SVG to PNG", (done) => {
        function callback(base64: string) {
            expect(base64).toContain("data:image/png;base64,")
            done()
        }

        new SVGConverter().convertToPngBase64(slackSVG, callback)
    })
})
