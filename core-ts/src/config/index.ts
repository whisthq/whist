import { keys, difference } from "lodash"

// We expect that configuration will be passed through the CONFIG environment
// variable. If the CONFIG environment variable is undefined, we'll replace it
// with an empty object.

// We should place all keys that we want type-safety from into this map. The
// default values don't matter, we'll just be using them for their types
// anyways.
export const DefaultConfig = {
    WEBSERVER_URL: "",
    AUTH_DOMAIN_URL: "", //fractal-dev.us.auth0.com
    AUTH_CLIENT_ID: "", //j1toifpKVu5WA6YbhEqBnMWN0fFxqw5I
    AUTH_API_IDENTIFIER: "", //https://api.fractal.co
    CLIENT_CALLBACK_URL: "", //http://localhost/callback
}

export type Config = typeof DefaultConfig

// This can be used as a type guard so that Typescript is aware that we
// have a full configuration object.
export const isConfig = (obj: any): obj is Config => {
    for (let key in DefaultConfig) {
        let value = DefaultConfig[key as keyof typeof DefaultConfig]
        if (!(key in obj)) return false
        if (!(typeof obj[key] === typeof value)) return false
    }
    return true
}

export const config = (() => {
    const c = JSON.parse(process.env.CONFIG ?? "{}")
    if (isConfig(c)) return c
    const missing = difference(keys(DefaultConfig), keys(c))
    console.log("Environment CONFIG=", c)
    throw new Error(`Environment CONFIG= missing required keys: ${missing}`)
})()
