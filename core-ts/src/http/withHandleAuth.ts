import { ServerDecorator } from "../types/api"
import { USER_LOGGED_OUT } from "../constants/errors"

/*
 * Accepts a "server" parameter at configuration time.
 * Returns a decorator that attaches a server to a ServerRequest.
 *
 * @param server - a string representing a HTTP server
 * @returns a ServerDecorator function
 */
export const withHandleAuth =
    (handler: Function): ServerDecorator =>
    async (fn, req) => {
        const result = await fn(req)
        if (result.error === USER_LOGGED_OUT) handler(result)
        return result
    }
