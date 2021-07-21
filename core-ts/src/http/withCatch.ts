import { ServerDecorator, ServerResponse } from "../types/api";

/*
 * Catches any errors downstream in a decorator pipeline.
 * Caught erorrs will be returned upstream on the error key.
 *
 * @param fn - The downstream decorator function
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const withCatch: ServerDecorator = async (fn, req) => {
  try {
    return await fn(req);
  } catch (err) {
    return {
      request: req,
      error: err,
      response: { status: 500 },
    } as ServerResponse;
  }
};
