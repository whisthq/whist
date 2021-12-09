import * as auth from "../"

describe("authInfoParse", () => {
  // These tokens were generated using https://dinochiesa.github.io/jwt.
  // Please use a similar tool to manually decode/update these tokens. The symmetric
  // encryption key is `Key-Must-Be-at-least-32-bytes-in-length!`.

  // This token encodes things like the subscription status.
  const testAccessTokenGood =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJodHRwczovL2FwaS5mcmFjdGFsLmNvL3N1YnNjcmlwdGlvbl9zdGF0dXMiOm51bGx9.6XU_EXIffw32nU21on_jkDBJ-w1F8pGMr11R38pDYvA"
  // This token encodes things like the user's email address.
  const testIDTokenGood =
    "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJXaGlzdCBUZXN0IEpXSyBCdWlsZGVyIiwiaWF0IjoxNjI2Mjc1OTQ2LCJleHAiOjE2NTc4MTE5NDYsImF1ZCI6Ind3dy53aGlzdC5jb20iLCJzdWIiOiJ0ZXN0LXVzZXIiLCJlbWFpbCI6InRlc3QtdXNlckB3aGlzdC5jb20iLCJodHRwczovL2FwaS5mcmFjdGFsLmNvL3N1YnNjcmlwdGlvbl9zdGF0dXMiOiJjYW5jZWxlZCJ9.TYRuQ2E5yDJWbXfJrBSI_O7rIuXVD79tkvBAYOnIGNU"
  // Matches the previous token, but is missing the email address.
  const testIdTokenBadEmail =
    "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJXaGlzdCBUZXN0IEpXSyBCdWlsZGVyIiwiaWF0IjoxNjI2Mjc1OTQ2LCJleHAiOjE2NTc4MTE5NDYsImF1ZCI6Ind3dy53aGlzdC5jb20iLCJzdWIiOiJ0ZXN0LXVzZXIiLCJodHRwczovL2FwaS5mcmFjdGFsLmNvL3N1YnNjcmlwdGlvbl9zdGF0dXMiOiJjYW5jZWxlZCJ9.nBvJ3k_xUqt0d6IDM2qZfhcDnOcbTq51hxjW-goaAXo"

  const testJSONGood = {
    id_token: testIDTokenGood,
    access_token: testAccessTokenGood,
  }

  const testJSONBadAccess = { ...testJSONGood, access_token: undefined }
  const testJSONBadEmail = { ...testJSONGood, id_token: testIdTokenBadEmail }

  test("returns userEmail, accessToken, subscriptionStatus", () => {
    expect(auth.authInfoParse({ json: testJSONGood })).toStrictEqual({
      userEmail: "test-user@whist.com",
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
