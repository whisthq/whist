import { Response } from "node-fetch"

export class FractalAPI {
  static TOKEN = {
    REFRESH: "/token/refresh",
    VALIDATE: "/token/validate",
  }

  static MANDELBOX = {
    ASSIGN: "/mandelbox/assign",
  }

  static MAIL = {
    FEEDBACK: "/mail/feedback",
  }

  static ACCOUNT = {
    LOGIN: "/account/login",
  }
}

export enum FractalHTTPCode {
  SUCCESS = 200,
  ACCEPTED = 202,
  PARTIAL_CONTENT = 206,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  PAYMENT_REQUIRED = 402,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  INTERNAL_SERVER_ERROR = 500,
}

export enum FractalHTTPRequest {
  POST = "POST",
  GET = "GET",
  PUT = "PUT",
  DELETE = "DELETE",
}

export enum FractalHTTPContent {
  JSON = "application/json",
  TEXT = "application/text",
}

type ServerRequestSchema = {
  url: string
  token: string
  mode: string
  body: string | Record<string, any>
  endpoint: string
  method: string
  server: string
  accessToken: string
  refreshToken: string
}

export type ServerRequest = Partial<ServerRequestSchema>

type ServerResponseSchema = {
  request: ServerRequest
  error: Error | string
  json: Record<string, any>
  text: string
}

export type ServerResponse = Partial<ServerResponseSchema> & {
  response?: Readonly<Response>
}

export type ServerEffect = (r: ServerRequest) => Promise<ServerResponse>

export type ServerDecorator = (
  f: ServerEffect,
  r: ServerRequest
) => Promise<ServerResponse>

export type ConfigHTTP = {
  server: string
  endpointRefreshToken?: string
  handleAuth?: Function
  debug?: boolean
}
