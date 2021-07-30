import { withHandleAuth } from "../withHandleAuth"
import { decorate } from "../../utilities"
import { USER_LOGGED_OUT } from "../../constants/errors"
import * as mock from "./mocks"

describe("withHandleAuth", () => {
    const input = mock.REQUEST_SUCCESS

    test("handler is called on error", async () => {
        // We're just checking if the decorated function
        // calls the handler, so we don't need to mock fetch
        const handler = jest.fn()
        const mockResponse = async (_: any) => {
            const m = await mock.FETCH_IDENTITY
            return { ...m, error: USER_LOGGED_OUT }
        }
        // @ts-ignore
        const get = decorate(mockResponse, withHandleAuth(handler))
        await get(input)
        expect(handler).toHaveBeenCalledTimes(1)
    })

    test("handler is not called with no error", async () => {
        const handler = jest.fn()
        // @ts-ignore
        const get = decorate(mock.FETCH_IDENTITY, withHandleAuth(handler))
        await get(input)
        expect(handler).not.toHaveBeenCalled()
    })

    test("return value has input unchanged", async () => {
        // This decorator should not change the request or response.
        const handler = jest.fn()
        const get = decorate(mock.FETCH_IDENTITY, withHandleAuth(handler))
        const result = await get(input)
        expect(result).toMatchObject(input)
    })
})
