const { Client } = require("@notionhq/client")
const fs = require("fs")
const path = require("path/posix")
const request = require("request-promise")

const NOTION_API_KEY = "secret_IWKwfhedzKrvbiWtCF7XcbxuX9emMwyfWNoAXfnQXNZ" // This is read-only
const NOTION_TEAM_DATABASE_ID = "f023b39bd511459d822fadbe2cf5b605" // ID of our Team Directory Notion database
const NOTION_INVESTOR_DATABASE_ID = "76ab133fe155432e9b1c7868b5d9d08a" // ID of our Investor Directory Notion database
const NOTION_DATA_DIRECTORY = "assets/company" // Where to save .json file containing database info (e.g. team member names)

// Initialize Notion SDK
const notion = new Client({ auth: NOTION_API_KEY })

const fetchTeamData = async () => {
  await notion.databases
    .query({
      database_id: NOTION_TEAM_DATABASE_ID,
      sorts: [
        {
          property: "Name",
          direction: "ascending",
        },
      ],
    })
    .then((db) =>
      Promise.all(
        db.results.map(async (row) => {
          const pageContents = await notion.pages.retrieve({
            page_id: row?.id,
          })
          await request({
            uri: pageContents?.properties?.Photo?.files?.[0]?.file?.url,
            encoding: null,
          }).then((body) => {
            const filepath = path.join(
              "public",
              NOTION_DATA_DIRECTORY,
              `${row.id}.png`
            )
            fs.writeFileSync(filepath, body)
          })

          return {
            name: pageContents?.properties?.Name?.title?.[0]?.plain_text ?? "", // e.g. Marcus Aurelius
            title:
              pageContents?.properties?.Title?.rich_text?.[0]?.plain_text ?? "", // e.g. Roman emperor
            imagePath: path.posix.join(NOTION_DATA_DIRECTORY, `${row.id}.png`), // e.g. assets/company/{row.id}.png
          }
        })
      )
    )
    .then((team) => {
      const jsonTarget = path.join("public", NOTION_DATA_DIRECTORY, "team.json")
      fs.writeFileSync(jsonTarget, JSON.stringify(team))
    })
}

const fetchInvestorData = async () => {
  await notion.databases
    .query({
      database_id: NOTION_INVESTOR_DATABASE_ID,
      sorts: [
        {
          property: "Name",
          direction: "ascending",
        },
      ],
    })
    .then((db) =>
      Promise.all(
        db.results.map(async (row) => {
          const pageContents = await notion.pages.retrieve({
            page_id: row?.id,
          })
          await request({
            uri: pageContents?.properties?.Photo?.files?.[0]?.file?.url,
            encoding: null,
          }).then((body) => {
            const filepath = path.join(
              "public",
              NOTION_DATA_DIRECTORY,
              `${row.id}.png`
            )
            fs.writeFileSync(filepath, body)
          })

          return {
            name: pageContents?.properties?.Name?.title?.[0]?.plain_text ?? "", // e.g. Sam Lessin
            website: pageContents?.properties?.Website?.url ?? "", // e.g. https://slow.co
            description:
              pageContents?.properties?.Description?.rich_text?.[0]
                ?.plain_text ?? "", // e.g. "Great Investor"
            imagePath: path.posix.join(NOTION_DATA_DIRECTORY, `${row.id}.png`), // e.g. assets/company/{row.id}.png
          }
        })
      )
    )
    .then((team) => {
      const jsonTarget = path.join(
        "public",
        NOTION_DATA_DIRECTORY,
        "investors.json"
      )
      fs.writeFileSync(jsonTarget, JSON.stringify(team))
    })
}

module.exports = {
  fetchCompanyInfo: async () => {
    fs.rmSync(path.join("public", NOTION_DATA_DIRECTORY), {
      recursive: true,
      force: true,
    })
    fs.mkdirSync(path.join("public", NOTION_DATA_DIRECTORY))
    await Promise.all([fetchTeamData(), fetchInvestorData()])
  },
}
