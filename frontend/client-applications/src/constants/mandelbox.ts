import { AWSRegion } from "@app/@types/aws"

export const regions = [
  {
    name: AWSRegion.US_EAST_1,
    enabled: true,
    territory: "United States",
  },
  {
    name: AWSRegion.US_EAST_2,
    enabled: true,
    territory: "United States",
  },
  {
    name: AWSRegion.US_WEST_1,
    enabled: true,
    territory: "United States",
  },
  {
    name: AWSRegion.US_WEST_2,
    enabled: true,
    territory: "United States",
  },
  {
    name: AWSRegion.CA_CENTRAL_1,
    enabled: true,
    territory: "Canada",
  },
  {
    name: AWSRegion.AF_SOUTH_1,
    enabled: false,
    territory: "South Africa",
  },
  {
    name: AWSRegion.AP_EAST_1,
    enabled: false,
    territory: "Hong Kong",
  },
  {
    name: AWSRegion.AP_SOUTHEAST_3,
    enabled: false,
    territory: "Indonesia",
  },
  {
    name: AWSRegion.AP_SOUTH_1,
    enabled: false,
    territory: "India",
  },
  {
    name: AWSRegion.AP_NORTHEAST_3,
    enabled: false,
    territory: "Japan",
  },
  {
    name: AWSRegion.AP_NORTHEAST_2,
    enabled: false,
    territory: "Korea",
  },
  {
    name: AWSRegion.AP_SOUTHEAST_1,
    enabled: false,
    territory: "Singapore",
  },
  {
    name: AWSRegion.AP_SOUTHEAST_2,
    enabled: false,
    territory: "Australia",
  },
  {
    name: AWSRegion.AP_NORTHEAST_1,
    enabled: false,
    territory: "Japan",
  },
  {
    name: AWSRegion.EU_CENTRAL_1,
    enabled: false,
    territory: "Germany",
  },
  {
    name: AWSRegion.EU_WEST_1,
    enabled: false,
    territory: "Ireland",
  },
  {
    name: AWSRegion.EU_WEST_2,
    enabled: false,
    territory: "England",
  },
  {
    name: AWSRegion.EU_SOUTH_1,
    enabled: false,
    territory: "Italy",
  },
  {
    name: AWSRegion.EU_WEST_3,
    enabled: false,
    territory: "France",
  },
  {
    name: AWSRegion.EU_NORTH_1,
    enabled: false,
    territory: "Sweden",
  },
  {
    name: AWSRegion.ME_SOUTH_1,
    enabled: false,
    territory: "Middle East",
  },
  {
    name: AWSRegion.SA_EAST_1,
    enabled: false,
    territory: "Brazil",
  },
]

export const HostServicePort = 4678
