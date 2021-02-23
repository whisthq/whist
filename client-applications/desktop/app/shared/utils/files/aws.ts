import ping from "ping"

import { graphQLPost } from "shared/utils/general/api"
import { allowedRegions, AWSRegion } from "shared/types/aws"
import { QUERY_REGION_TO_AMI } from "shared/constants/graphql"
import { config } from "shared/constants/config"
import { FractalLogger } from "shared/utils/general/logging"

const logger = new FractalLogger()

const fractalPingTime = async (host: string, numberPings: number) => {
    /*
    Description:
        Measures the average ping time (in ms) to ping a host (IP address or URL)

    Arguments:
        host (string): IP address or URL
        numberPings (number): Number of times to ping the host
    Returns:
        (number): Average ping time to ping host (in ms)
    */

    // Create list of Promises, where each Promise resolves to a ping time
    const pingPromises = []
    for (let i = 0; i < numberPings; i += 1) {
        pingPromises.push(ping.promise.probe(host))
    }

    // Resolve list of Promises synchronously to get a list of ping outputs
    const pingResults = await Promise.all(pingPromises)

    // Calculate the average ping time
    let totalTime = 0
    for (let i = 0; i < numberPings; i += 1) {
        totalTime += Number(pingResults[i].avg)
    }

    return totalTime / pingResults.length
}

export const setAWSRegion = async () => {
    /*
    Description:
        Pulls AWS regions from SQL and pings each region, and finds the closest region
        by shortest ping time

    Arguments:
        none
    Returns:
        (string): Closest region e.g. us-east-1
    */

    // Pull allowed regions from SQL via GraphQL
    let finalRegions = []
    const graphqlRegions = await graphQLPost(
        QUERY_REGION_TO_AMI,
        "GetRegionToAmi",
        {}
    )

    // Default to hard-coded region list if GraphQL query fails, otherwise parse
    // GraphQL query output
    if (
        !graphqlRegions ||
        !graphqlRegions.json ||
        !graphqlRegions.json.data ||
        !graphqlRegions.json.data.hardware_region_to_ami
    ) {
        finalRegions = allowedRegions
    } else {
        finalRegions = graphqlRegions.json.data.hardware_region_to_ami.map(
            (region: { region_name: AWSRegion }) => region.region_name
        )
    }

    // Ping each region and find the closest region by lowest ping time
    let closestRegion = finalRegions[0]
    let lowestPingTime = Number.MAX_SAFE_INTEGER

    /* eslint-disable no-await-in-loop */
    for (let i = 0; i < finalRegions.length; i += 1) {
        const region = finalRegions[i]
        const averagePingTime = await fractalPingTime(
            `dynamodb.${region}.amazonaws.com`,
            3
        )
        logger.logInfo(`It took ${averagePingTime.toString()} to ping ${region}`)
        
        if (averagePingTime < lowestPingTime) {
            closestRegion = region
            lowestPingTime = averagePingTime
        }
    }

    return closestRegion
}

export const uploadToS3 = (
    localFilePath: string,
    s3FileName: string,
    callback: (error: string) => void,
    accessKey = config.keys.AWS_ACCESS_KEY,
    secretKey = config.keys.AWS_SECRET_KEY,
    bucketName = "fractal-protocol-logs"
) => {
    /*
    Description:
        Uploads a local file to S3

    Arguments:
        localFilePath (string): Path of file to upload (e.g. C://log.txt)
        s3FileName (string): What to call the file once it's uploaded to S3 (e.g. "FILE.txt")
        callback (function): Callback function to fire once file is uploaded
    Returns:
        boolean: Success true/false
    */

    const fs = require("fs")
    const AWS = require("aws-sdk")
    const s3 = new AWS.S3({
        accessKeyId: accessKey,
        secretAccessKey: secretKey,
    })
    // Read file into buffer
    try {
        const fileContent = fs.readFileSync(localFilePath)

        // Set up S3 upload parameters
        const params = {
            Bucket: bucketName,
            Key: s3FileName,
            Body: fileContent,
        }

        // Upload files to the bucket
        s3.upload(params, (s3Error: string) => {
            callback(s3Error)
        })
    } catch (unknownErr) {
        callback(unknownErr)
    }
}
