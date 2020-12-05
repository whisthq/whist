/* eslint-disable */

import { SVGConverter } from "shared/utils/shortcuts"

/* Define test variables here */
jest.setTimeout(30000)

// SVGConverter() variables
let slackSVG =
    "https://fractal-supported-app-images.s3.amazonaws.com/slack-256.svg"

/* Begin tests */
describe("shortcuts.ts", () => {
    test("SVGConverter(): Convert valid SVG to PNG", (done) => {
        function callback(error: Error, buffer: any) {
            expect(error).toBeNull()
            expect(buffer).not.toBeNull()
            done()
        }

        SVGConverter(slackSVG, callback)
    })
})
