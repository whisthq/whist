/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file api.ts
 * @brief This file contains utility functions for HTTP requests.
 */

import stringify from "json-stringify-safe"

const httpConfig =
  (method: string) =>
  async (args: { body?: object; url: string; accessToken?: string }) => {
    let response = null

    try {
      response = await fetch(args.url, {
        method,
        headers: {
          "Content-Type": "application/json",
          ...(args.accessToken !== undefined && {
            Authorization: `Bearer ${args.accessToken}`,
          }),
        },
        ...(args.body !== undefined && {
          body: stringify(args.body),
        }),
      })
    } catch (err) {
      return {
        status: 500,
        json: {},
      }
    }

    try {
      const json = await response.json()
      return {
        status: response?.status ?? 500,
        json,
      }
    } catch (err) {
      return {
        status: response?.status ?? 500,
        json: {},
      }
    }
  }

const post = httpConfig("POST")
const put = httpConfig("PUT")
const get = httpConfig("GET")

export { post, put, get }
