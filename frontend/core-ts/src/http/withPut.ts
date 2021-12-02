import { ServerDecorator } from "../types/api"

/*
 * Attaches a PUT method to a ServerRequest.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const withPut: ServerDecorator = async (fn, req) =>
  await fn({ ...req, method: "PUT" })
