import _config from "@app/config.json"

export const config = JSON.parse(JSON.stringify(_config))
export const sessionID = Date.now()
