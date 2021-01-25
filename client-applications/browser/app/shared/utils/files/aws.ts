import { debugLog } from "shared/utils/general/logging"
import { graphQLPost } from "shared/utils/general/api"
import { allowedRegions } from "shared/types/aws"
import { fractalBackoff } from "shared/utils/general/helpers"
import { QUERY_REGION_TO_AMI } from "shared/constants/graphql"
import { OperatingSystem, FractalDirectory } from "shared/types/client"
import { config } from "shared/constants/config"

export const setAWSRegion = (accessToken: string, backoff?: true) => {
    /*
    Description:
        Runs AWS ping shell script (tells us which AWS server is closest to the client)

    Arguments:
        accessToken (string): Access token, used to query GraphQL for allowed regions
        backoff (boolean): If true, uses exponential backoff w/ jitter in case AWS ping fails
    Returns:
        promise : Promise
    */

    const awsPromise = () => {
        return new Promise((resolve, reject) => {
            const { spawn } = require("child_process")
            const platform = require("os").platform()
            const path = require("path")
            const fs = require("fs")

            let executableName
            const binariesPath = path.join(
                FractalDirectory.getRootDirectory(),
                "binaries"
            )

            if (platform === OperatingSystem.MAC) {
                executableName = "awsping_osx"
            } else if (platform === OperatingSystem.WINDOWS) {
                executableName = "awsping_windows.exe"
            } else if (platform === OperatingSystem.LINUX) {
                executableName = "awsping_linux"
            } else {
                debugLog(`no suitable os found, instead got ${platform}`)
            }

            const executablePath = path.join(binariesPath, executableName)

            fs.chmodSync(executablePath, 0o755)

            const regions = spawn(executablePath, ["-n", "3"]) // ping via TCP
            regions.stdout.setEncoding("utf8")

            regions.stdout.on("data", async (data: string) => {
                // Gets the line with the closest AWS region, and replace all instances of multiple spaces with one space
                const output = data.split(/\r?\n/)
                let finalRegions = null
                const graphqlRegions = await graphQLPost(
                    QUERY_REGION_TO_AMI,
                    "GetRegionToAmi",
                    {},
                    accessToken
                )

                if (
                    !graphqlRegions ||
                    !graphqlRegions.json ||
                    !graphqlRegions.json.data ||
                    !graphqlRegions.json.data.hardware_region_to_ami
                ) {
                    finalRegions = allowedRegions
                } else {
                    finalRegions = graphqlRegions.json.data.hardware_region_to_ami.map(
                        (region: { region_name: string }) => region.region_name
                    )
                }

                let index = 0
                let line = output[index].replace(/  +/g, " ")
                let items = line.split(" ")

                try {
                    items[2].slice(1, -1)
                } catch (err) {
                    reject(new Error("AWS ping failed"))
                }

                while (
                    !finalRegions.includes(items[2].slice(1, -1)) &&
                    index < output.length - 1
                ) {
                    index += 1
                    line = output[index].replace(/  +/g, " ")
                    items = line.split(" ")
                }
                // In case data is split and sent separately, only use closest AWS region which has index of 0
                const region = items[2].slice(1, -1)
                resolve(region)
            })
            return null
        })
    }

    if (backoff) {
        return fractalBackoff(awsPromise)
    }
    return awsPromise()
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
        void
    */

    const fs = require("fs")
    const AWS = require("aws-sdk")
    const s3 = new AWS.S3({
        accessKeyId: accessKey,
        secretAccessKey: secretKey,
    })
    // Read file into buffer
    const fileContent = fs.readFileSync(localFilePath)

    // Set up S3 upload parameters
    const params = {
        Bucket: bucketName,
        Key: s3FileName,
        Body: fileContent,
    }

    // Upload files to the bucket
    s3.upload(params, (error: string) => {
        callback(error)
    })
}
