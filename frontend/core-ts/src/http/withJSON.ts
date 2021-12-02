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
  // Certain badly formed responses, like internal server 500,
  // will throw an error if you call the response.json() method.
  // This can happen even though a json method exists on the
  // response object.
  //
  // This is a unusual case, so we'll choose to catch the error
  // and simply leave the json key unassigned when passing the
  // result upstream.
  //
  // The json method is still available on the response key,
  // which is holding the original response object, in case
  // the user wants to try for themselves.
  if (response?.json)
    try {
      return { response, json: await response.json() }
    } catch (e) {}
  return { response }
}
