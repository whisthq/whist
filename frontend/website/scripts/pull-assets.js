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

// The main function that actually runs
// Queries Notion API and pull all data
;(async () => {
  console.log("Pulling assets from Notion database...")

  const savedInvestorInfo = await pullInvestorInfo()
  const savedTeamInfo = await pullTeamInfo()
  const jsonDirectory = `./${NOTION_DATA_DIRECTORY}/notion.json`

  fs.writeFileSync(
    jsonDirectory,
    JSON.stringify({
      team: savedTeamInfo,
      investors: savedInvestorInfo,
    })
  )
  console.log(`Retrieved Notion data and saved at ${jsonDirectory}`)
})()
