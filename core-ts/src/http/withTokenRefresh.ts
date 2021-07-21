import { USER_LOGGED_OUT } from "../constants/errors";
import {
  ServerDecorator,
  ServerRequest,
  ServerEffect,
  ServerResponse,
} from "../types/api";
import { fetchBase } from "./fetchBase";
import { withJSON } from "./withJSON";
import { withPost } from "./withPost";
import { withURL } from "./withURL";
import { decorate } from "../utilities";

const post = decorate(fetchBase, withPost, withURL, withJSON);

/*
 * Test for the shape of a refresh or access token.
 *
 * @param token - any type, a valid token is a string
 * @returns true if token shape is valid, false otherwise
 */
const isToken = (token: unknown) => !(!token || token === "");

/*
 * Handles access to protected resources.
 * Caught 401 status code and handles freshing tokens.
 * Retries the original request once after refresh.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */

export const withTokenRefresh =
  (endpoint: string): ServerDecorator =>
  async (fn: ServerEffect, req: ServerRequest) => {
    const { refreshToken, accessToken } = req;
    const firstResponse = await fn({ ...req, token: accessToken });

    // Only deal with 401 response.
    if (firstResponse.response?.status !== 401) return firstResponse;

    // If we have no access code or refresh code, we're not logged in.
    if (!isToken(accessToken) || !isToken(refreshToken))
      return { ...firstResponse, error: USER_LOGGED_OUT } as ServerResponse;

    // We have a refresh token, ask for a new access code.
    const refreshResponse = await post({ endpoint, token: refreshToken });
    const newAccess = refreshResponse.json && refreshResponse.json.accessToken;

    // If we still don't have an access code, we send back up an error.
    if (!isToken(newAccess))
      return { ...firstResponse, error: USER_LOGGED_OUT };

    return await fn({ ...req, token: newAccess });
  };
