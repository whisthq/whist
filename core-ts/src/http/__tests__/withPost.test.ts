import { withPost } from "../withPost"
import { decorate } from "../../utilities"
import * as mock from "./mocks"

describe("withPost", () => {
    const input = mock.REQUEST_SUCCESS
    // We can pass an identity function to avoid
    // mocking fetch
    const apiFn = decorate(mock.FETCH_IDENTITY, withPost)
    const result = apiFn(input)

    test("return value has post method", async () => {
        expect(await result).toHaveProperty("method", "POST")
    })
    test("return value has input unchanged", async () => {
        expect(await result).toMatchObject(input)
    })
})
