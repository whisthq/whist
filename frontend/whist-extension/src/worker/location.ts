import { getCountry, getSortedAWSRegions } from "@app/utils/location"

import { WorkerMessageType } from "@app/constants/messaging"
import { regions } from "@app/constants/location"
import { createEvent } from "@app/utils/events"

const initAWSRegionPing = async () => {
  const country = getCountry()
  const nonUSRegionsAllowed = country !== "US"
  const sortedRegions = await getSortedAWSRegions(
    regions
      .filter(
        (region) =>
          region.enabled &&
          (nonUSRegionsAllowed ? true : region.country === "US")
      )
      .map((region) => region.name)
  )

  createEvent(WorkerMessageType.CLOSEST_AWS_REGIONS, sortedRegions)
}

export { initAWSRegionPing }
