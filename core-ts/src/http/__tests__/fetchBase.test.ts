import { fetchBase } from "../fetchBase"
import * as mock from "./mocks"
import fetchMock from "jest-fetch-mock"

beforeEach(() => fetchMock.doMock())

describe("fetchBase", () => {
    // Create a mock repsonse to be returned by fetch
    const mockedBody = "test-body"
    const mockedRequest = mock.GET_REQUEST_SUCCESS
    const mockedResponse = mock.MOCK_RESPONSE
    // Mock fetch to return the above response
    fetchMock.mockResponse(mockedBody, mockedResponse)
    const result = fetchBase(mockedRequest)

    test("return value has body", async () => {
        const r = await result
        const response = r.response
        expect(await response?.text()).toBe(mockedBody)
    })

    test("return value has request object", async () => {
        const { request } = await result
        expect(request?.url).toBe(mockedRequest.url)
        expect(request?.token).toBe(mockedRequest.token)
    })

    test("returns value has ok, request, status", async () => {
        const { request, response } = await result
        expect(response?.ok).toEqual(mockedResponse.ok)
        expect(response?.status).toEqual(mockedResponse.status)
        expect(request).toEqual(mockedRequest)
    })
})
