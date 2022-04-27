import { of } from "rxjs"
import { filter, switchMap } from "rxjs/operators"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import { sortRegionByProximity, getCountry } from "@app/main/utils/location"
import { flow } from "@app/main/utils/flows"
import { persistGet } from "@app/main/utils/persist"

import { regions } from "@app/constants/location"
import { AWS_REGIONS_SORTED_BY_PROXIMITY } from "@app/constants/store"

export default flow("awsPingFlow", (trigger) => {
  const cachedRegions = of(persistGet(AWS_REGIONS_SORTED_BY_PROXIMITY)).pipe(
    filter((regions) => (regions ?? undefined) !== undefined)
  )

  const currentCountry = getCountry()
  const nonUSRegionsAllowed = currentCountry !== "US"

  const pingedRegions = trigger.pipe(
    switchMap(
      async () =>
        await sortRegionByProximity(
          regions
            .filter(
              (region) =>
                region.enabled &&
                (nonUSRegionsAllowed ? true : region.country === "US")
            )
            .map((region) => region.name)
        )
    )
  )

  return {
    cached: cachedRegions,
    refresh: pingedRegions.pipe(
      filter((regions) =>
        isEmpty(pickBy(regions, (x) => x.pingTime === undefined))
      )
    ),
    offline: pingedRegions.pipe(
      filter(
        (regions) => !isEmpty(pickBy(regions, (x) => x.pingTime === undefined))
      )
    ),
  }
})
