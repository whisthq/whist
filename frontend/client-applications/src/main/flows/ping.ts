import { of } from "rxjs"
import { filter, switchMap } from "rxjs/operators"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import { sortRegionByProximity } from "@app/main/utils/region"
import { flow } from "@app/main/utils/flows"
import {
  defaultAllowedRegions,
  defaultUSRegions,
} from "@app/constants/mandelbox"
import { persistGet } from "@app/main/utils/persist"
import {
  ALLOW_NON_US_SERVERS,
  AWS_REGIONS_SORTED_BY_PROXIMITY,
} from "@app/constants/store"

export default flow("awsPingFlow", (trigger) => {
  const cachedRegions = of(persistGet(AWS_REGIONS_SORTED_BY_PROXIMITY)).pipe(
    filter((regions) => (regions ?? undefined) !== undefined)
  )

  // By default we don't allow non-US AWS regions because websites like Netflix
  // will serve the Canadian version of the website to a US visitor, and most of our
  // users are in the US. We split by US vs. non-US instead of by specific country
  // because currently all our servers are in the US with the exception of ca-central-1.
  // Once we enable more AWS regions can change this to be per country.
  const nonUSRegionsAllowed =
    (persistGet(ALLOW_NON_US_SERVERS) as boolean) ?? false

  const pingedRegions = trigger.pipe(
    switchMap(
      async () =>
        await sortRegionByProximity(
          nonUSRegionsAllowed ? defaultAllowedRegions : defaultUSRegions
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
