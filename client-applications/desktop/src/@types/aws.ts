export enum AWSRegion {
    US_EAST_1 = "us-east-1",
    US_EAST_2 = "us-east-2",
    US_WEST_1 = "us-west-1",
    US_WEST_2 = "us-west-2",
    CA_CENTRAL_1 = "ca-central-1",
    EU_CENTRAL_1 = "eu-central-1",
}

export const defaultAllowedRegions = [
    AWSRegion.US_EAST_1,
    AWSRegion.US_EAST_2,
    AWSRegion.US_WEST_1,
    AWSRegion.US_WEST_2,
    AWSRegion.CA_CENTRAL_1,
    AWSRegion.EU_CENTRAL_1,
]
