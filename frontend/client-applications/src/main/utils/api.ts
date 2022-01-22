/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file api.ts
 * @brief This file contains utility functions to make API calls.
 */

import { configGet, configPost, configPut } from "@whist/core-ts"
import config from "@app/config/environment"
import { sessionID } from "@app/constants/app"

import { logging } from "@app/main/utils/logging"

/*
 * @whist/core-ts http functions like "get" and "post"
 * are constructed at runtime so they can be passed a config
 * object, like the httpConfig object below.
 *
 * Naming across config objects will be consistent as core-ts grows.
 * Common prefixes will include:
 *
 * handle - A function that will accept a request or response object, to
 * be called for its side effects upon certain events, like error.
 *
 * endpoint - A string with the format "/api/endpoint", will be
 * concatenated onto a server prefix to form a complete URL.
 *
 */

const httpConfig = {
  server: config.url.WEBSERVER_URL,
  // handleAuth: (_: any) => goTo("/auth"),
  endpointRefreshToken: "/token/refresh",
}

export const withSessionID = (body: object) => {
  return {
    ...body,
    session_id: sessionID,
  }
}

export const get = (...args: any) => {
  logging("GET request sent", args)
  return configGet(httpConfig)(...args)
}

export const post = (...args: any) => {
  logging("POST request sent", args)
  return configPost(httpConfig)(...args)
}
export const hostPut =
  (server: string) =>
  (...args: any) => {
    logging("PUT request sent", args)
    return configPut({
      server,
    })(...args)
  }

// TODO: this needs to move somewhere else, but we're not using it yet
export const tokenValidate = async (accessToken: string) =>
  get({ endpoint: "/token/validate", accessToken })
