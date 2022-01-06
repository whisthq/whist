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

  let savedInvestorInfo = await notion.fetchNotionInvestorData()
  let savedTeamInfo = await notion.fetchNotionTeamData()

  if (environment !== "local") {
    const teamS3Directory = `${environment}/team`
    const investorS3Directory = `${environment}/investors`

    await aws.emptyS3Directory(AWS_BUCKET_NAME, s3Directory)

    savedTeamInfo.forEach((info, i) => {
      savedTeamInfo[i].imageUrl = await aws.uploadUrlToS3(
        info.imageUrl,
        `${teamS3Directory}/${info.name}.png`
      )
    })

    savedInvestorInfo.forEach((info, i) => {
      savedInvestorInfo[i].imageUrl = await aws.uploadUrlToS3(
        info.imageUrl,
        `${investorS3Directory}/${info.name}.png`
      )
    })
  }

  notion.saveDataLocally({
    team: savedTeamInfo,
    investors: savedInvestorInfo,
  })

  console.log("Assets pulled and cached successfully")
})()
