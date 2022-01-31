import applescript from "applescript"
import range from "lodash.range"

import { InstalledBrowser } from "@app/constants/importer"

const getNumberOfBrowserWindows = async (browser: string) => {
  const script = `tell application "${browser}" to get number of windows`

  return await new Promise((resolve, reject) => {
    applescript.execString(script, (err: any, raw: string) => {
      if (err !== undefined) reject(err)
      resolve(raw)
    })
  })
}

const installedBrowserToApplescript = (browser: string) => {
  switch (browser) {
    case InstalledBrowser.CHROME:
      return "Google Chrome"
    case InstalledBrowser.BRAVE:
      return "Brave"
    case InstalledBrowser.OPERA:
      return "Opera"
    default:
      return "Google Chrome"
  }
}

const getOtherBrowserWindows = async (browser: string) => {
  browser = installedBrowserToApplescript(browser)
  const numberOfWindows = await getNumberOfBrowserWindows(browser)
  const windows = []

  for (const i of range(1, Number(numberOfWindows) + 1)) {
    const getUrls = `
        set urls to ""

        tell application "${browser}"
            set window_list to every window
                
            set the_window to item ${i} of window_list
            set tab_list to every tab in the_window
            
            repeat with the_tab in tab_list 
                    set the_url to the URL of the_tab
                    set urls to urls & the_url & "\n"
            end repeat
        end tell
    `

    const getTitle = `
        set the_title to ""

        tell application "${browser}"
            set window_list to every window
            set the_window to item ${i} of window_list
            set the_title to the title of active tab of the_window
        end tell
    `

    const urls = await new Promise((resolve, reject) => {
      applescript.execString(getUrls, (err: any, raw: string) => {
        if (err !== undefined) reject(err)

        const urls = []

        for (const url of raw?.split("\n")) {
          if (url !== "") urls.push(url)
        }

        resolve(urls)
      })
    })

    const title = await new Promise((resolve, reject) => {
      applescript.execString(getTitle, (err: any, title: string) => {
        if (err !== undefined) reject(err)
        resolve(title)
      })
    })

    windows.push({ id: i, urls, title })
  }
  return windows
}

export { getOtherBrowserWindows }
