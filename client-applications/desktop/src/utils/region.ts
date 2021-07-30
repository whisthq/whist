/**
 * Copyright Fractal Computers, Inc. 2021
 * @file region.ts
 * @brief This file contains utility functions for finding the closest AWS region.
 */

import ping from "ping"
import { values } from "lodash"

import { AWSRegion } from "@app/@types/aws"

const findLowest = (arr: number[]) => {
  let min = Number.MAX_SAFE_INTEGER
  for (let i = 0; i < arr.length; i += 1) {
    if (arr[i] < min) {
      min = arr[i]
    }
  }
  return min
}

const fractalPingTime = async (host: string, numberPings: number) => {
  /*
    Description:
        Measures the average ping time (in ms) to ping a host (IP address or URL)

    Arguments:
        host (string): IP address or URL
        numberPings (number): Number of times to ping the host
    Returns:
        (number): Average ping time to ping host (in ms)
    */

  // Create list of Promises, where each Promise resolves to a ping time
  const pingPromises = []
  for (let i = 0; i < numberPings; i += 1) {
    pingPromises.push(ping.promise.probe(host))
  }

  // Resolve list of Promises synchronously to get a list of ping outputs
  const pingResults = await Promise.all(pingPromises)
  const pingTimes = pingResults.map((res) => Number(res.avg))

  return findLowest(pingTimes)
}

export const chooseRegion = async (regions: AWSRegion[]) => {
  /*
    Description:
        Pulls AWS regions from SQL and pings each region, and finds the closest region
        by shortest ping time

    Arguments:
        none
    Returns:
        (string): Closest region e.g. us-east-1
    */

  // Ping each region and find the closest region by lowest ping time
  let closestRegion = regions[0]
  let lowestPingTime = Number.MAX_SAFE_INTEGER

  /* eslint-disable no-await-in-loop */
  for (let i = 0; i < regions.length; i += 1) {
    const region = regions[i]
    const averagePingTime = await fractalPingTime(
      `dynamodb.${region}.amazonaws.com`,
      3
    )

    if (averagePingTime < lowestPingTime) {
      closestRegion = region
      lowestPingTime = averagePingTime
    }
  }

  return closestRegion
}

export const getRegionFromArgv = (argv: string[]) => {
  return (values(AWSRegion) as string[]).includes(argv[argv.length - 1])
    ? argv[argv.length - 1]
    : undefined
}
