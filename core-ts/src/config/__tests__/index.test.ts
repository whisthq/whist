import * as idx from "../"

describe("isConfig", () => {
    // @ts-expect-error
    idx.DefaultConfig = {
        WEBSERVER_URL: "",
        AUTH_URL: "",
    }
    const good = {
        WEBSERVER_URL: "http://website.com",
        AUTH_URL: "http://auth.com",
    }
    const bad = {
        WEBSERVER_URL: "http://website.com",
    }
    const extra = {
        WEBSERVER_URL: "http://website.com",
        AUTH_URL: "http://auth.com",
        EXTRA_URL: "http://extra.com",
    }

    test("returns true when all DefaultConfig keys present", () => {
        expect(idx.isConfig(good)).toBe(true)
    })

    test("returns false when any DefaultConfig keys missing", () => {
        expect(idx.isConfig(bad)).toBe(false)
    })

    test("allows extra keys that are not in DefaultConfig", () => {
        expect(idx.isConfig(extra)).toBe(true)
    })
})
