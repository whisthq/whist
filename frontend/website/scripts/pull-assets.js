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

// Don't worry, this secret key has read-only permissions to our Team Directory
const NOTION_API_KEY = "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ"
// ID of our Team Directory Notion database
const NOTION_DATABASE_ID = "f023b39bd511459d822fadbe2cf5b605"
// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })

const downloadImageLocally = (imageUrl, name) => {
  const http = require("http")
  const https = require("https")
  const Stream = require("stream").Transform
  const fs = require("fs")

  const url = new URL(imageUrl)
  const client = url.protocol === "https:" ? https : http

  client
    .request(imageUrl, function (response) {
      let data = new Stream()

      response.on("data", function (chunk) {
        data.push(chunk)
      })

      response.on("end", function () {
        fs.writeFileSync(`./public/assets/team/${name}.jpg`, data.read())
        console.log(`Created file ${name}.jpg`)
      })
    })
    .end()
}

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

    // Download image locally
    downloadImageLocally(imageUrl, name.replace(/\s/g, ""))
  })
})()
