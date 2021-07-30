import { ServerEffect, ServerResponse } from "../../types/api"

export const REQUEST_SUCCESS = {
  url: "https://fractal.co/endpoint",
  token: "accessToken",
}

export const ENDPOINT = "ENDPOINT"
export const SERVER = "SERVER"

export const GET_REQUEST_SUCCESS = { ...REQUEST_SUCCESS, method: "GET" }

export const MOCK_RESPONSE = {
  body: async () => {},
  error: () => Error(""),
  redirect: (_: any, __: any) => {},
  clone: (_: any) => {},
  headers: {},
  ok: true,
  redirected: false,
  status: 200,
  statusTest: "",
  type: null,
  url: "",
}

export const RESPONSE_SUCCESS = {
  response: { ...MOCK_RESPONSE, status: 200, ok: true },
  request: GET_REQUEST_SUCCESS,
}

// We can use this function as the first argument to decorate
// to avoid having to call fetchBase, thus avoiding mocking fetch
export const FETCH_IDENTITY: ServerEffect = async (x) =>
  ({ ...x, request: x } as ServerResponse)
