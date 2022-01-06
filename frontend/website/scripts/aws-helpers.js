const AWS = require("aws-sdk")
const request = require("request-promise")

const AWS_BUCKET_NAME = "whist-website-assets" // Where to upload website assets
const s3 = new AWS.S3({
  accessKeyId: process.env.AWS_ACCESS_KEY_ID ?? "AKIA24A776SSDGVALR5K",
  secretAccessKey:
    process.env.AWS_SECRET_KEY ?? "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",
})

module.exports = {
  emptyS3Directory: async (dir) => {
    const _emptyS3Directory = async (bucket, dir) => {
      const listedObjects = await s3
        .listObjectsV2({
          Bucket: bucket,
          Prefix: dir,
        })
        .promise()

      if (listedObjects.Contents.length === 0) return

      const deleteParams = {
        Bucket: bucket,
        Delete: { Objects: [] },
      }

      listedObjects.Contents.forEach(({ Key }) => {
        deleteParams.Delete.Objects.push({ Key })
      })

      await s3.deleteObjects(deleteParams).promise()

      if (listedObjects.IsTruncated) await _emptyS3Directory(bucket, dir)
    }

    await _emptyS3Directory(AWS_BUCKET_NAME, dir)
  },
  uploadUrlToS3: async (url, filepath) => {
    try {
      const body = await request({
        uri: url,
        encoding: null,
      })

      const { Location } = await s3
        .upload({
          Bucket: AWS_BUCKET_NAME,
          Key: filepath,
          Body: body,
        })
        .promise()

      return Location
    } catch (err) {
      return undefined
    }
  },
}
