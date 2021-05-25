import fs from "fs"
import path from "path"
import AWS from "aws-sdk"
import config, { loggingBaseFilePath } from "@app/config/environment"

// Upload a local file to s3.
const uploadToS3 = async (s3FileName: string, localFilePath: string) => {
  const s3 = new AWS.S3({
    accessKeyId: config.keys.AWS_ACCESS_KEY,
    secretAccessKey: config.keys.AWS_SECRET_KEY,
  })
  // Set up S3 upload parameters
  const params = {
    Bucket: "fractal-protocol-logs",
    Key: s3FileName,
    Body: fs.readFileSync(localFilePath),
  }
  // Upload files to the bucket
  return await new Promise((resolve, reject) => {
    s3.upload(params, (err: Error, data: any) => {
      if (err !== null) reject(err)
      else resolve(data)
    })
  })
}

// Given a s3 file name, upload local logs to s3.
export const uploadLogsToS3 = async (email: string) => {
  const s3FileName = `CLIENT_${email}_${new Date().getTime()}.txt`
  const logLocations = [
    path.join(loggingBaseFilePath, "log-dev.txt"),
    path.join(loggingBaseFilePath, "log-staging.txt"),
    path.join(loggingBaseFilePath, "log.txt"),
  ]

  const uploadPromises = logLocations
    .filter(fs.existsSync)
    .map((filePath: string) => uploadToS3(s3FileName, filePath))

  await Promise.all(uploadPromises).catch((err) => {
    console.log("ERROR uploading logs to AWS S3:")
    console.log(err)
  })
}
