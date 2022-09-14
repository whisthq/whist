/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file location.ts
 * @brief This file contains utility functions for finding the closest AWS region.
 */

import sortBy from "lodash.sortby"

import { AWSRegion, regions, timezones } from "@app/constants/location"

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
  const pingResults = [] as number[]
  for (let i = 0; i < numberPings; i += 1) {
    try {
      const start = Date.now()
      await fetch(host)
      pingResults.push(Date.now() - start)
    } catch (err: any) {
      throw new Error(err)
    }
  }

  // Resolve list of Promises synchronously to get a list of ping outputs
  if (pingResults.length > 0)
    return pingResults.reduce((a, b) => Math.min(a, b))
  return undefined
}

const pingLoop = (regions: AWSRegion[]) => {
  // Ping each region and find the closest region by lowest ping time
  const pingResultPromises = []
  /* eslint-disable no-await-in-loop */
  for (let i = 0; i < regions.length; i += 1) {
    const region = regions[i]

    pingResultPromises.push(
      whistPingTime(`http://ec2.${region}.amazonaws.com/ping`, 6).then(
        (pingTime) => {
          return { region, pingTime }
        }
      )
    )
  }
  return pingResultPromises
}

const getSortedAWSRegions = async (regions: AWSRegion[]) => {
  /*
    Description:
      Pulls AWS regions from SQL and pings each region, and sorts regions
      by shortest ping time

    Arguments:
      (AWSRegion[]): Unsorted array of regions
    Returns:
      (AWSRegion[]): Sorted array of regions
  */

  try {
    const pingResults = await Promise.all(pingLoop(regions))
    return sortBy(pingResults, ["pingTime"])
  } catch (err: any) {
    throw new Error(err)
  }
}

const getCountry = () => {
  const timezone =
    Intl.DateTimeFormat()?.resolvedOptions()?.timeZone ?? undefined

  if (timezone === undefined) return undefined

  return timezones[timezone]?.c?.[0] ?? undefined
}

const initAWSRegionPing = async () => {
  const country = getCountry()
  const nonUSRegionsAllowed = country !== "US"

  try {
    const sortedRegions = await getSortedAWSRegions(
      regions
        .filter(
          (region) =>
            region.enabled &&
            (nonUSRegionsAllowed ? true : region.country === "US")
        )
        .map((region) => region.name)
    )
    return sortedRegions
  } catch (err: any) {
    throw new Error(err)
  }
}

export { initAWSRegionPing }
