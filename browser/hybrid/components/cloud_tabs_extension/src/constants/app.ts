import _config from "@app/generated_config.json"

export const config = JSON.parse(JSON.stringify(_config))
export const sessionID = Date.now()
export const inviteCode = "94478"