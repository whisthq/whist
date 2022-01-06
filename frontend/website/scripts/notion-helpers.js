const { Client } = require("@notionhq/client")
const fs = require("fs")

const NOTION_API_KEY = "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ" // This is read-only, don't worry
const NOTION_TEAM_DATABASE_ID = "f023b39bd511459d822fadbe2cf5b605" // ID of our Team Directory Notion database
const NOTION_INVESTOR_DATABASE_ID = "76ab133fe155432e9b1c7868b5d9d08a" // ID of our Investor Directory Notion database
const NOTION_DATA_DIRECTORY = "public/assets/metadata" // Where to save .json file containing database info e.g. team member names

// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })

module.exports = {
  fetchNotionTeamData: async () => {
    const teamDatabase = await notion.databases.query({
      database_id: NOTION_TEAM_DATABASE_ID,
      sorts: [
        {
          property: "Name",
          direction: "ascending",
        },
      ],
    })

    let savedTeamInfo = []

    // Iterate through database entries
    for (const row of teamDatabase?.results) {
      const pageContents = await notion.pages.retrieve({
        page_id: row?.id,
      })
      const imageUrl =
        pageContents?.properties?.Photo?.files?.[0]?.file?.url ?? "" // e.g. http://link-to-image.png
      const name = pageContents?.properties?.Name?.title?.[0]?.plain_text ?? "" // e.g. Marcus Aurelius
      const title =
        pageContents?.properties?.Title?.rich_text?.[0]?.plain_text ?? "" // e.g. Roman emperor

      savedTeamInfo.push({ name, title, imageUrl })
    }

    return savedTeamInfo
  },
  fetchNotionInvestorData: async () => {
    const investorDatabase = await notion.databases.query({
      database_id: NOTION_INVESTOR_DATABASE_ID,
      sorts: [
        {
          property: "Name",
          direction: "ascending",
        },
      ],
    })

    let savedInvestorInfo = []

    // Iterate through database entries
    for (const row of investorDatabase?.results) {
      const pageId = row?.id
      const pageContents = await notion.pages.retrieve({
        page_id: pageId,
      })
      const imageUrl =
        pageContents?.properties?.Photo?.files?.[0]?.file?.url ?? "" // e.g. http://link-to-image.png
      const name = pageContents?.properties?.Name?.title?.[0]?.plain_text ?? "" // e.g. Sam Lessin
      const website = pageContents?.properties?.Website?.url ?? "" // e.g. e.g. http://slow.co
      const description =
        pageContents?.properties?.Description?.rich_text?.[0]?.plain_text ?? "" // e.g. "Great Investor"

      savedInvestorInfo.push({ name, imageUrl, website, description })
    }

    return savedInvestorInfo
  },
  saveDataLocally: (obj) => {
    const jsonDirectory = `./${NOTION_DATA_DIRECTORY}/notion.json`
    fs.writeFileSync(jsonDirectory, JSON.stringify(obj))
  },
}
