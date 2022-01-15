import { from, of } from "rxjs"
import { filter } from "rxjs/operators"

import { sortRegionByProximity } from "@app/main/utils/region"
import { flow } from "@app/main/utils/flows"
import { defaultAllowedRegions } from "@app/constants/mandelbox"
import { persistGet } from "@app/main/utils/persist"
import { AWS_REGIONS_SORTED_BY_PROXIMITY } from "@app/constants/store"

export default flow("awsPingFlow", () => {
  const cachedRegions = of(persistGet(AWS_REGIONS_SORTED_BY_PROXIMITY)).pipe(
    filter((regions) => (regions ?? undefined) !== undefined)
  )

  const pingedRegions = from(sortRegionByProximity(defaultAllowedRegions))

  return {
    cached: cachedRegions,
    refresh: pingedRegions,
  }
})
