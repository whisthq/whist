import { getCountry, getSortedAWSRegions } from "@app/worker/utils/location"

import { ContentScriptMessageType } from "@app/constants/messaging"
import { regions } from "@app/constants/location"

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

  chrome.runtime.sendMessage({
    type: ContentScriptMessageType.CLOSEST_AWS_REGIONS,
    value: sortedRegions,
  })
}

export { initAWSRegionPing }
