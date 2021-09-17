/**
 * Copyright Fractal Computers, Inc. 2021
 * @file region.ts
 * @brief This file contains utility functions for finding the closest AWS region.
 */

import { values } from "lodash"

import { AWSRegion } from "@app/@types/aws"
import fetch from "node-fetch"

const fractalPingTime = async (host: string, numberPings: number) => {
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
  const pingPromises = []
  for (let i = 0; i < numberPings; i += 1) {
    const startTime = Date.now()
    pingPromises.push(fetch(host).then(() => Date.now() - startTime))
  }

  // Resolve list of Promises synchronously to get a list of ping outputs
  const pingResults = await Promise.all(pingPromises)
  return pingResults.reduce((a, b) => a < b ? a : b)
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
  const pingResultPromises = []
  /* eslint-disable no-await-in-loop */
  for (let i = 0; i < regions.length; i += 1) {
    const region = regions[i]
    const randomHash = Math.floor(Math.random() * Math.pow(2, 52)).toString(36)
    const endpoint = `/does-not-exist?cache-break=${randomHash}`
    pingResultPromises.push(fractalPingTime(
      `http://dynamodb.${region}.amazonaws.com${endpoint}`,
      3
    ).then((pingTime) => { return { region, pingTime } }))
  }
  const pingResults = await Promise.all(pingResultPromises)
  const closestRegion = pingResults.reduce((a, b) => a.pingTime < b.pingTime ? a : b).region
  console.log(pingResults)
  console.log(`closest region is ${closestRegion}`)

  return closestRegion
}

export const getRegionFromArgv = (argv: string[]) => {
  return (values(AWSRegion) as string[]).includes(argv[argv.length - 1])
    ? argv[argv.length - 1]
    : undefined
}
