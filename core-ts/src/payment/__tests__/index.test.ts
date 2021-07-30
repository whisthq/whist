import * as auth from "../"

describe("paymentPortalParse", () => {
    const good = {
        json: { url: "http://website.com" },
    }
    const badNumber = {
        json: { url: 10 },
    }

    const badNull = {
        json: null,
    }
    test("returns portalPortalURL if url is found", () => {
        expect(auth.paymentPortalParse(good)).toStrictEqual({
            paymentPortalURL: "http://website.com",
        })
    })
    test("returns error and data if url is not found", () => {
        const result = auth.paymentPortalParse(badNull)
        expect(result).toHaveProperty("error")
        expect(result.error).toHaveProperty("message")
    })
    test("returns error and data if url is not a string", () => {
        const result = auth.paymentPortalParse(badNumber)
        expect(result).toHaveProperty("error")
        expect(result.error).toHaveProperty("message")
    })
})
