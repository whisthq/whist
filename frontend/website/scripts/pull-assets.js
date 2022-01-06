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
const notion = require("./notion-helpers")
const aws = require("./aws-helpers")

// Queries Notion API, saves images to S3, and caches results
;(async () => {
  console.log("Pulling assets from Notion database...")

  const environment = process.env.WHIST_ENVIRONMENT ?? "local"

  // Pull investor and team data from Notion
  const savedInvestorInfo = await notion.fetchNotionInvestorData()
  const savedTeamInfo = await notion.fetchNotionTeamData()

  // This is meant to run in dev/staging/prod CI deployments
  // Upload the pulled Notion images to S3 so they're saved forever
  if (environment !== "local") {
    console.log("Uploading assets to S3...")

    const teamS3Directory = `${environment}/team`
    const investorS3Directory = `${environment}/investors`

    await aws.emptyS3Directory(teamS3Directory)
    await aws.emptyS3Directory(investorS3Directory)

    for (let i = 0; i < savedTeamInfo.length; i++) {
      const info = savedTeamInfo[i]
      savedTeamInfo[i].imageUrl = await aws.uploadUrlToS3(
        info.imageUrl,
        `${teamS3Directory}/${info.name}.png`
      )
    }

    for (let i = 0; i < savedInvestorInfo.length; i++) {
      const info = savedInvestorInfo[i]
      savedInvestorInfo[i].imageUrl = await aws.uploadUrlToS3(
        info.imageUrl,
        `${investorS3Directory}/${info.name}.png`
      )
    }
  }

  // Save the Notion data as a file so that it can be read by the website
  console.log(savedTeamInfo)
  console.log(savedInvestorInfo)

  notion.saveDataLocally({
    team: savedTeamInfo,
    investors: savedInvestorInfo,
  })

  console.log("Assets pulled and cached successfully")
})()
