/* eslint-disable */

import { searchArrayByKey, updateArrayByKey } from "shared/utils/helpers"
import { FractalAppLocalState } from "shared/types/ui"

/* Define test variables here */

// searchArrayByKey() and updateArrayByKey() variables
let chrome = { app_id: "Chrome", localState: FractalAppLocalState.INSTALLED }
let figma = { app_id: "Figma", localState: FractalAppLocalState.NOT_INSTALLED }
let blender = {
    app_id: "Blender",
    localState: FractalAppLocalState.NOT_INSTALLED,
}

let appArray = [chrome, figma, blender]

/* Begin tests */
describe("helpers.ts", () => {
    // searchArrayByKey() tests
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

    // updateArrayByKey() tests
    test("updateArrayByKey(): If valid object is replaced with a valid key/value pair", () => {
        const { array, index } = updateArrayByKey(
            appArray,
            "app_id",
            "Blender",
            {
                localState: FractalAppLocalState.INSTALLED,
            }
        )
        // Check to see that new array is correctly modified
        expect(index).toEqual(2)
        expect(array[index].localState).toEqual(FractalAppLocalState.INSTALLED)
        // Check to see that original array was not modified
        expect(appArray[index].localState).toEqual(
            FractalAppLocalState.NOT_INSTALLED
        )
    })
    test("updateArrayByKey(): If an invalid key is searched for", () => {
        // Check to see that new array is correctly modified
        expect(() =>
            updateArrayByKey(appArray, "invalidKey", "Blender", {
                localState: FractalAppLocalState.INSTALLED,
            })
        ).toThrow()
    })
})
