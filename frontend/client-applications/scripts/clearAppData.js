/*
  Clears electron user data cache

  To be called via:
  $ yarn cache:clear --[dev|staging|prod|all]
*/

const path = require("path")
const { app } = require("electron")
const fs = require("fs-extra")

const clearAppData = (_env, ...args) => {
  let folderToDelete

  switch (args[0]) {
    case "--local":
      folderToDelete = "local"
      break
    case "--dev":
      folderToDelete = "dev"
      break
    case "--staging":
      folderToDelete = "staging"
      break
    case "--prod":
      folderToDelete = "prod"
      break
    case "--all":
      folderToDelete = "."
      break
  }

  if (!folderToDelete) {
    console.log("Must specify a flag: --[local|dev|staging|prod|all]")
    app.exit()
  }

  const appPath = path.join(
    app.getPath("appData"),
    require("../package.json").productName,
    folderToDelete
  )
  console.log(`Deleting ${appPath}`)
  fs.rmdirSync(appPath, { recursive: true })

  app.exit()
}

module.exports = clearAppData

if (require.main === module) {
  clearAppData({}, ...process.argv.slice(2))
}
