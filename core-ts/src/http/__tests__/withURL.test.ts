import { withURL } from "../withURL"
import { ServerRequest } from "../../types/api"
import { decorate } from "../../utilities"
import * as mock from "./mocks"

describe("withURL", () => {
    // Pass an identity function to decorate
    // to avoid mocking fetch
    const input = mock.REQUEST_SUCCESS
    const get = decorate(mock.FETCH_IDENTITY, withURL)

    test("return value has url", async () => {
        expect(await get(input)).toHaveProperty("url", input.url)
    })

    test("return value has error on missing parameters", async () => {
        const req = { ...input } as ServerRequest
        delete req["url"]
        delete req["endpoint"]

        expect(await get(req)).toHaveProperty("error")
    })
    test("return value has input unchanged", async () => {
        const result = await get(input)
        expect(result.request).toMatchObject(input)
    })
})
