import { ServerDecorator } from "../types/api"

/*
 * Validates the status code of a ServerResponse.
 * Returns a ServerResponse with ok set to true
 * if valid, false if not.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const withStatus: ServerDecorator = async (fn, req) => {
    const result = await fn(req)
    return {
        ...result,
        status: result.response?.status,
        statusText: result.response?.statusText,
    }
}
