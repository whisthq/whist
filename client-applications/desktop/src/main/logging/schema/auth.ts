import { pick, omit, get, mapValues, truncate } from "lodash"
import { LoggingFormat, LoggingSchema } from "@app/main/logging/utils"

const truncateValues = (obj: { [key: string]: string }) =>
  mapValues(obj, (s) => truncate(s, { length: 16 }))

const outputJsonTokens: LoggingFormat = [
  "only truncated tokens",
  (val: object) =>
    truncateValues(
      pick(get(val, "json"), [
        "access_token, refresh_token",
        "encrypted_config_token",
      ])
    ),
]

const outputTokensTruncated: LoggingFormat = [
  "truncating tokens",
  (val: { [key: string]: string }) => ({
    ...val,
    ...truncateValues(
      pick(val, ["accessToken", "refreshToken", "configToken"])
    ),
  }),
]

const schema: LoggingSchema = {
  loginRequest: {
    success: outputJsonTokens,
  },
  loginFlow: {
    success: outputTokensTruncated,
  },
  accessTokenRequest: {
    success: outputTokensTruncated,
  },
}

export default schema
