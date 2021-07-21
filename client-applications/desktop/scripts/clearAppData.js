/* 
  Clears electron user data cache

  To be called via:
  $ yarn cache:clear -- [all|dev|prod]
*/

const path = require("path")
const { app } = require("electron")
const fs = require("fs-extra")
const { userDataFolderNames } = require("../config/configs")

const clearAppData = (_env, ...args) => {
    let foldersToDelete

    switch (args[0]) {
        case "--all":
            foldersToDelete = Object.values(userDataFolderNames)
            break
        case "--prod":
            foldersToDelete = [userDataFolderNames.production]
            break
        case "--dev":
            foldersToDelete = [userDataFolderNames.development]
            break
    }

    if (!foldersToDelete) {
        console.log("Must specify a flag: --[all|dev|prod]")
        app.exit()
    }

    foldersToDelete.forEach((folder) => {
        const appPath = path.join(app.getPath("appData"), folder)
        console.log("Clearing", appPath)
        fs.rmdirSync(appPath, { recursive: true })
    })

    app.exit()
}

module.exports = clearAppData

if (require.main === module) {
    clearAppData({}, ...process.argv.slice(2))
}
