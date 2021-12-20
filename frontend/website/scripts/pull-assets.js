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
// Where to save .json file containing database info e.g. team member names
const TEAM_DATA_DIRECTORY = "public/assets/metadata"
// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })

// The main function that actually runs
// Queries Notion API and pull all data
;(async () => {
  console.log("Pulling assets from Notion database...")

  const database = await notion.databases.query({
    database_id: NOTION_DATABASE_ID,
    sorts: [
      {
        property: "Name",
        direction: "ascending",
      },
    ],
  })

  let savedTeamInfo = []

  // Iterate through database entries
  await database?.results?.forEach(async (row) => {
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

    // Finally, save the info
    savedTeamInfo.push({
      name,
      title,
      imageUrl,
    })

    if (savedTeamInfo.length === database.results.length) {
      fs.writeFileSync(
        `./${TEAM_DATA_DIRECTORY}/notion.json`,
        JSON.stringify({
          team: savedTeamInfo,
        })
      )
      console.log(
        `Retrieved Notion data and created ./${TEAM_DATA_DIRECTORY}/notion.json`
      )
    }
  })
})()
