import { ServerDecorator, WhistHTTPCode } from "../types/api"

/*
 * Validates the status code of a ServerResponse.
 * Returns a ServerResponse with ok set to true
 * if valid, false if not.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const withStatusCheck: ServerDecorator = async (fn, req) => {
  const result = await fn(req)
  const { response } = result
  return response?.status === WhistHTTPCode.SUCCESS ||
    response?.status === WhistHTTPCode.ACCEPTED
    ? result
    : { ...result, error: "Failure Status: " + response?.status }
}
