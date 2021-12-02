import { ServerDecorator } from "../types/api"

/*
 * Adds 'status' and 'statusText' keys to the
 * top-level response object. Returns the rest
 * of the ServerResponse unchanged.
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
