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
const http = require("http")
const https = require("https")
const Stream = require("stream").Transform
const fs = require("fs")

// Don't worry, this secret key has read-only permissions to our Team Directory
const NOTION_API_KEY = "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ"
// ID of our Team Directory Notion database
const NOTION_DATABASE_ID = "f023b39bd511459d822fadbe2cf5b605"
// Team photos directory, relative to root of the website subrepo
const TEAM_PHOTOS_DIRECTORY = "public/assets/team"
// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })
// Function to save image locally
const downloadImageLocally = (
  imageUrl,
  name,
  directory = TEAM_PHOTOS_DIRECTORY
) => {
  const url = new URL(imageUrl)
  const client = url.protocol === "https:" ? https : http

  client
    .request(imageUrl, function (response) {
      let data = new Stream()

      response.on("data", function (chunk) {
        data.push(chunk)
      })

      response.on("end", function () {
        fs.writeFileSync(`./${directory}/${name}.jpg`, data.read())
        console.log(`Saved image ${directory}/${name}.jpg`)
      })
    })
    .end()
}

// Query Notion API and pull all data
;(async () => {
  const database = await notion.databases.query({
    database_id: NOTION_DATABASE_ID,
    sorts: [
      {
        property: "Name",
        direction: "ascending",
      },
    ],
  })

  // Before looping through the database entries, create a clean folder
  // to store the team photos
  const teamPhotosDirectoryExists = fs.existsSync(TEAM_PHOTOS_DIRECTORY)
  if (teamPhotosDirectoryExists)
    fs.rmSync(TEAM_PHOTOS_DIRECTORY, { recursive: true })
  fs.mkdirSync(TEAM_PHOTOS_DIRECTORY)
  console.log(`Directory ${TEAM_PHOTOS_DIRECTORY} created`)

  database?.results?.forEach(async (row) => {
    const pageId = row?.id
    const pageContents = await notion.pages.retrieve({
      page_id: pageId,
    })
    // Team member AWS S3 hosted headshot URL
    const imageUrl =
      pageContents?.properties?.Photo?.files?.[0]?.file?.url ?? ""
    // Team member name e.g. Marcus Aurelius
    const name = pageContents?.properties?.Name?.title?.[0]?.plain_text
    // Team member title e.g. Roman Emperor
    const title = pageContents?.properties?.Title?.rich_text?.[0]?.plain_text

    // Now we have pulled all the data, it's time to save it locally
    downloadImageLocally(imageUrl, name.replace(/\s/g, ""))
  })
})()
