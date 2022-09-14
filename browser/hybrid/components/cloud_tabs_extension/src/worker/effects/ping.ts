import { initAWSRegionPing } from "@app/worker/utils/location"
import { setStorage } from "@app/worker/utils/storage"

import { Storage } from "@app/constants/storage"

// Find closest AWS region
void initAWSRegionPing()
  .then((regions) => {
    void setStorage(Storage.CLOSEST_AWS_REGIONS, regions)
  })
  .catch((err) => {
    console.error("No Internet", err)
  })
