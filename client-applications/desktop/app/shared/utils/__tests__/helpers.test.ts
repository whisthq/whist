/* eslint-disable */

import { searchArrayByKey } from "shared/utils/helpers"
import { FractalAppLocalState } from "shared/types/ui"

/* Define test variables here */

// searchArrayByKey() variables
let chrome = { app_id: "Chrome", localState: FractalAppLocalState.INSTALLED }
let figma = { app_id: "Figma", localState: FractalAppLocalState.NOT_INSTALLED }
let blender = {
    app_id: "Blender",
    localState: FractalAppLocalState.NOT_INSTALLED,
}

let appArray = [chrome, figma, blender]

// Begin tests
describe("helpers.ts", () => {
    test("searchArrayByKey(): If valid object is found", () => {
        const { value, index } = searchArrayByKey(appArray, "app_id", "Blender")
        expect(value).toEqual(blender)
        expect(index).toEqual(2)
    })
    test("searchArrayByKey(): If valid object is not found", () => {
        const { value, index } = searchArrayByKey(appArray, "app_id", "Adobe")
        expect(value).toEqual(null)
        expect(index).toEqual(-1)
    })
    test("searchArrayByKey(): If key does not exist, throw an error", () => {
        expect(() => {
            searchArrayByKey(appArray, "nonexistent_key", "Adobe")
        }).toThrow()
    })
})
