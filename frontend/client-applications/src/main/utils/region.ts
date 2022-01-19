/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file region.ts
 * @brief This file contains utility functions for finding the closest AWS region.
 */

import fetch from "node-fetch"
import sortBy from "lodash.sortby"

import { AWSRegion } from "@app/@types/aws"
import { logBase } from "@app/main/utils/logging"

const whistPingTime = async (host: string, numberPings: number) => {
  /*
    Description:
        Measures the average ping time (in ms) to ping a host (IP address or URL)

    Arguments:
        url (string): IP address or URL
        numberPings (number): Number of times to ping the host
    Returns:
        (number): Average ping time to ping host (in ms)
    */

  // Create list of Promises, where each Promise resolves to a ping time
  const pingResults = []
  for (let i = 0; i < numberPings; i += 1) {
    const startTime = Date.now()

    await fetch(host)

    pingResults.push(Date.now() - startTime)
  }

  // Resolve list of Promises synchronously to get a list of ping outputs
  return pingResults.reduce((a, b) => Math.min(a, b))
}

const pingLoop = (regions: AWSRegion[]) => {
  // Ping each region and find the closest region by lowest ping time
  const pingResultPromises = []
  /* eslint-disable no-await-in-loop */
  for (let i = 0; i < regions.length; i += 1) {
    const region = regions[i]
    const randomHash = Math.floor(Math.random() * Math.pow(2, 52)).toString(36)
    const endpoint = `/does-not-exist?cache-break=${randomHash}`
    pingResultPromises.push(
      whistPingTime(
        `http://dynamodb.${region}.amazonaws.com${endpoint}`,
        6
      ).then((pingTime) => {
        return { region, pingTime }
      })
    )
  }
  return pingResultPromises
}

const sortRegionByProximity = async (regions: AWSRegion[]) => {
  /*
  Description:
      Pulls AWS regions from SQL and pings each region, and sorts regions
      by shortest ping time

  Arguments:
      (AWSRegion[]): Unsorted array of regions
  Returns:
      (AWSRegion[]): Sorted array of regions
  */
  const pingResults = await Promise.all(pingLoop(regions))

  const sortedResults = sortBy(pingResults, ["pingTime"])

  logBase(`Sorted AWS regions are [${sortedResults.toString()}]`, {})

  return sortedResults
}

export { sortRegionByProximity }
