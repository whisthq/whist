import { isString } from "lodash";
import { ServerDecorator } from "../types/api";

/*
 * Builds a url using a server string and an endpoint string.
 *
 * @param server - a string representing a HTTP server
 * @param endpoint - a string representing a HTTP endpoint
 * @returns a string representing a HTTP request URL
 */
const serverUrl = (server: string, endpoint: string): string =>
  `${server}${endpoint}`;

/*
 * Ensures that a url is present on a ServerRequest object.
 * If a url is present, it passes on the object unchanged.
 * If no url is present, it attempts to build one using the
 * server and endpoint keys.
 * Without valid url, server, or endpoint keys, it will
 * stop the HTTP request and return a error ServerResponse.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const withURL: ServerDecorator = async (fn, req) => {
  const { url, server, endpoint } = req;

  if (isString(url)) {
    return await fn(req);
  } else {
    if (isString(server) && isString(endpoint)) {
      return await fn({
        ...req,
        url: serverUrl(server || "", endpoint || ""),
      });
    } else {
      return {
        request: req,
        error: Error(
          "Server API not configured with: url or [server, endpoint]."
        ),
      };
    }
  }
};
