import { ServerDecorator } from "../types/api"

/*
 * Awaits a JSON stream from the server, and passes the result
 * upstream in a decorator pipeline.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a Promise with the HTTP response
 */
export const withJSON: ServerDecorator = async (fn, req) => {
    const { response } = await fn(req)
    const json = (await response?.json()) || {}
    return { response, json }
}
