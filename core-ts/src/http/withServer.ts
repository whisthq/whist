import { ServerDecorator } from "../types/api";

/*
 * Accepts a "server" parameter at configuration time.
 * Returns a decorator that attaches a server to a ServerRequest.
 *
 * @param server - a string representing a HTTP server
 * @returns a ServerDecorator function
 */
export const withServer =
  (server: string): ServerDecorator =>
  async (fn, req) =>
    await fn({ ...req, server });
