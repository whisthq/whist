/* scripts/lint.js
 *
 * This script uses the shiny new Notion API
 * https://developers.notion.com/reference
 * to load our assets (i.e. team photos) into the website
 *
 * This should be invoked via:
 *   $ yarn pull:assetsß
 * from your command line.
 *
 */

const { Client } = require("@notionhq/client")
const fs = require("fs")
const AWS = require("aws-sdk")
const request = require("request-promise")

// Don't worry, this secret key has read-only permissions to our Team Directory
const NOTION_API_KEY = "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ"
// ID of our Team Directory Notion database
const NOTION_TEAM_DATABASE_ID = "f023b39bd511459d822fadbe2cf5b605"
// ID of our Investor Directory Notion database
const NOTION_INVESTOR_DATABASE_ID = "76ab133fe155432e9b1c7868b5d9d08a"
// Where to save .json file containing database info e.g. team member names
const NOTION_DATA_DIRECTORY = "public/assets/metadata"
// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })
// Initialize AWS SDk
const s3 = new AWS.S3({
  accessKeyId: process.env.AWS_ACCESS_KEY_ID ?? "AKIA24A776SSDGVALR5K",
  secretAccessKey:
    process.env.AWS_SECRET_KEY ?? "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",
})
const AWS_BUCKET_NAME = "whist-website-assets"

// Reads and returns info from Notion Team Directory
const pullTeamInfo = async () => {
  const savedTeamInfo = []

  const teamDatabase = await notion.databases.query({
    database_id: NOTION_TEAM_DATABASE_ID,
    sorts: [
      {
        property: "Name",
        direction: "ascending",
      },
    ],
  })

  // Iterate through database entries
  for (const row of teamDatabase?.results) {
    const pageId = row?.id
    const pageContents = await notion.pages.retrieve({
      page_id: pageId,
    })
    // Team member AWS S3 hosted headshot URL
    const imageUrl =
      pageContents?.properties?.Photo?.files?.[0]?.file?.url ?? ""
    // Team member name e.g. Marcus Aurelius
    const name = pageContents?.properties?.Name?.title?.[0]?.plain_text ?? ""
    // Team member title e.g. Roman Emperor
    const title =
      pageContents?.properties?.Title?.rich_text?.[0]?.plain_text ?? ""

    // Finally, save the info
    savedTeamInfo.push({
      name,
      title,
      imageUrl,
    })
  }

  return savedTeamInfo
}

// Reads and returns info from Notion Investor Directory
const pullInvestorInfo = async () => {
  const savedInvestorInfo = []

  const investorDatabase = await notion.databases.query({
    database_id: NOTION_INVESTOR_DATABASE_ID,
    sorts: [
      {
        property: "Name",
        direction: "ascending",
      },
    ],
  })

  // Iterate through database entries
  for (const row of investorDatabase?.results) {
    const pageId = row?.id
    const pageContents = await notion.pages.retrieve({
      page_id: pageId,
    })
    // Team member AWS S3 hosted headshot URL
    const imageUrl =
      pageContents?.properties?.Photo?.files?.[0]?.file?.url ?? ""
    // Team member name e.g. Marcus Aurelius
    const name = pageContents?.properties?.Name?.title?.[0]?.plain_text ?? ""
    // Investor website e.g. https://stoicsrus.com
    const website = pageContents?.properties?.Website?.url ?? ""
    // Team member title e.g. Roman Emperor
    const description =
      pageContents?.properties?.Description?.rich_text?.[0]?.plain_text ?? ""

    // Finally, save the info
    savedInvestorInfo.push({
      name,
      imageUrl,
      website,
      description,
    })
  }

  return savedInvestorInfo
}

const emptyS3Directory = async (bucket, dir) => {
  const listParams = {
    Bucket: bucket,
    Prefix: dir,
  }

  const listedObjects = await s3.listObjectsV2(listParams).promise()

  if (listedObjects.Contents.length === 0) return

  const deleteParams = {
    Bucket: bucket,
    Delete: { Objects: [] },
  }

  listedObjects.Contents.forEach(({ Key }) => {
    deleteParams.Delete.Objects.push({ Key })
  })

  await s3.deleteObjects(deleteParams).promise()
}

const uploadUrlToS3 = async (url, filepath) => {
  try {
    const options = {
      uri: url,
      encoding: null,
    }

    const body = await request(options)

    const { Location } = await s3
      .upload({
        Bucket: AWS_BUCKET_NAME,
        Key: filepath,
        Body: body,
      })
      .promise()

    return Location
  } catch (err) {
    console.error(`Upload failed for ${filepath}:`, err)
    return undefined
  }
}

// The main function that actually runs
// Queries Notion API and pull all data
;(async () => {
  console.log("Pulling assets from Notion database...")

  const environment = process.env.WHIST_ENVIRONMENT ?? "local"

  let savedInvestorInfo = await pullInvestorInfo()
  let savedTeamInfo = await pullTeamInfo()
  const jsonDirectory = `./${NOTION_DATA_DIRECTORY}/notion.json`

  await emptyS3Directory(AWS_BUCKET_NAME, "/")

  for (let i = 0; i < savedTeamInfo.length; i++) {
    const info = savedTeamInfo[i]
    if ((info.imageUrl ?? "") !== "") {
      const location = await uploadUrlToS3(
        info.imageUrl,
        `${environment}/team/${info.name}.png`
      )
      savedTeamInfo[i].imageUrl = location
    }
  }

  console.log(savedTeamInfo)

  fs.writeFileSync(
    jsonDirectory,
    JSON.stringify({
      team: savedTeamInfo,
      investors: savedInvestorInfo,
    })
  )
  console.log(`Retrieved Notion data and saved at ${jsonDirectory}`)
})()
