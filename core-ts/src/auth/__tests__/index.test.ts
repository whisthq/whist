import * as auth from "../"

describe("authInfoParse", () => {
  const testIDTokenGood =
    "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJPbmxpbmUgSldUIEJ1aWxkZXIiLCJpYXQiOjE2MjYyNzU5NDYsImV4cCI6MTY1NzgxMTk0NiwiYXVkIjoid3d3LmV4YW1wbGUuY29tIiwic3ViIjoianJvY2tldEBleGFtcGxlLmNvbSIsImVtYWlsIjoidGVzdEBmcmFjdGFsLmNvIiwiaHR0cHM6Ly9hcGkuZnJhY3RhbC5jby9zdWJzY3JpcHRpb25fc3RhdHVzIjoiY2FuY2VsZWQifQ.JvPevh0_pOWtgAlH2oZsX1HXF4m7fNA5pRFMt4d1glk"

  const testAccessTokenGood =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJteV91c2VybmFtZSIsImh0dHBzOi8vYXBpLmZyYWN0YWwuY28vc3Vic2NyaXB0aW9uX3N0YXR1cyI6bnVsbH0.7sa1hPGgOCbHTGpB4RS5PO2FjP20fkeCpI8Fgb6BQEQ"
  // Token decodes to:
  // {
  //     "iss": "Online JWT Builder",
  //     "iat": 1626275946,
  //     "exp": 1657811946,
  //     "aud": "www.example.com",
  //     "sub": "jrocket@example.com",
  //     "email": "test@fractal.co"
  //     "https://api.fractal.co/subscription_status": "canceled"
  // }

  // Decodes to same as above, but missing email field
  const testIdTokenBadEmail =
    "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJPbmxpbmUgSldUIEJ1aWxkZXIiLCJpYXQiOjE2MjYyNzU5NDYsImV4cCI6MTY1NzgxMTk0NiwiYXVkIjoid3d3LmV4YW1wbGUuY29tIiwic3ViIjoianJvY2tldEBleGFtcGxlLmNvbSIsImh0dHBzOi8vYXBpLmZyYWN0YWwuY28vc3Vic2NyaXB0aW9uX3N0YXR1cyI6ImNhbmNlbGVkIn0.qhaIQ5dAMgPNpXWi3geaTxSlZVCvfXs_-bRIe_8ldsk"

  const testJSONGood = {
    id_token: testIDTokenGood,
    access_token: testAccessTokenGood,
  }

  const testJSONBadAccess = { ...testJSONGood, access_token: undefined }
  const testJSONBadEmail = { ...testJSONGood, id_token: testIdTokenBadEmail }

  test("returns userEmail, accessToken, subscriptionStatus", () => {
    expect(auth.authInfoParse({ json: testJSONGood })).toStrictEqual({
      userEmail: "test@fractal.co",
      accessToken: testAccessTokenGood,
      subscriptionStatus: null,
    })
  })
  test("returns error if no access_token", () => {
    const result = auth.authInfoParse({ json: testJSONBadAccess })
    expect(result).toHaveProperty("error")
    expect(result.error).toHaveProperty("message")
  })
  test("returns error if no jwt email", () => {
    const result = auth.authInfoParse({ json: testJSONBadEmail })
    expect(result).toHaveProperty("error")
    expect(result.error).toHaveProperty("message")
  })
})
