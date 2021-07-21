import * as auth from "../"

describe("authInfoParse", () => {
    const testIDTokenGood =
        "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJPbmxpbmUgSldUIEJ1aWxkZXIiLCJpYXQiOjE2MjYyNzU5NDYsImV4cCI6MTY1NzgxMTk0NiwiYXVkIjoid3d3LmV4YW1wbGUuY29tIiwic3ViIjoianJvY2tldEBleGFtcGxlLmNvbSIsImVtYWlsIjoidGVzdEBmcmFjdGFsLmNvIn0.qrW6m1NKEbWGuuHkbdkghARGBTNVhGCvJhJrN3733aU"
    // Token decodes to:
    // {
    //     "iss": "Online JWT Builder",
    //     "iat": 1626275946,
    //     "exp": 1657811946,
    //     "aud": "www.example.com",
    //     "sub": "jrocket@example.com",
    //     "email": "test@fractal.co"
    // }

    // Decodes to same as above, but missing email field
    const testIdTokenBadEmail =
        "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJPbmxpbmUgSldUIEJ1aWxkZXIiLCJpYXQiOjE2MjYyNzU5NDYsImV4cCI6MTY1NzgxMTk0NiwiYXVkIjoid3d3LmV4YW1wbGUuY29tIiwic3ViIjoiIn0.4dyHXTvDX-Z-VPlqDytwUcNoE5jFrc7QiBS2SHDYaw8"

    const testJSONGood = {
        id_token: testIDTokenGood,
        access_token: "abcdefg",
        refresh_token: "hijklmnop",
    }

    const testJSONBadAccess = { ...testJSONGood, access_token: undefined }
    const testJSONBadRefresh = { ...testJSONGood, refresh_token: undefined }
    const testJSONBadEmail = { ...testJSONGood, id_token: testIdTokenBadEmail }

    test("returns jwtIdentity, userEmail, accessToken, refreshToken", () => {
        expect(auth.authInfoParse({ json: testJSONGood })).toStrictEqual({
            jwtIdentity: "jrocket@example.com",
            userEmail: "test@fractal.co",
            accessToken: "abcdefg",
            refreshToken: "hijklmnop",
        })
    })
    test("returns error if no access_token", () => {
        const result = auth.authInfoParse({ json: testJSONBadAccess })
        expect(result).toHaveProperty("error")
        expect(result.error).toHaveProperty("message")
    })
    test("returns error if no refresh_token", () => {
        const result = auth.authInfoParse({ json: testJSONBadRefresh })
        expect(result).toHaveProperty("error")
        expect(result.error).toHaveProperty("message")
    })
    test("returns error if no jwt email", () => {
        const result = auth.authInfoParse({ json: testJSONBadEmail })
        expect(result).toHaveProperty("error")
        expect(result.error).toHaveProperty("message")
    })
})
