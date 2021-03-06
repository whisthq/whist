/* eslint-disable */

import { FractalAPI } from "shared/types/api"
import { apiGet, apiPost, apiDelete } from "shared/utils/general/api"

/* Define test variables here */
const USERNAME = "not-a-valid-username"
const PASSWORD = "not-a-valid-password"
const ACCESS_TOKEN = "not-a-valid-access-token"

/* Begin tests */
describe("api.ts", () => {
    // searchArrayByKey() tests
    test("apiGet(): Send a GET request to Fractal webserver", async () => {
        const { json, success } = await apiGet(
            FractalAPI.TOKEN.VALIDATE,
            ACCESS_TOKEN
        )
        expect(success).toEqual(false)
        expect(json).toMatchObject({ msg: "Not enough segments" })
    })
    test("apiPost(): Send a POST request to Fractal webserver", async () => {
        const { json, success } = await apiPost(
            FractalAPI.ACCOUNT.LOGIN,
            {
                username: USERNAME,
                password: PASSWORD,
            },
            ACCESS_TOKEN
        )
        expect(success).toEqual(true)
        expect(json).toBeTruthy()
    })
    test("apiDelete(): Send a DELETE request to Fractal webserver", async () => {
        const { json, success } = await apiDelete(
            FractalAPI.APPS.CONNECTED + "/google_drive",
            ACCESS_TOKEN
        )
        expect(success).toEqual(false)
        expect(json).toBeTruthy()
    })
})
