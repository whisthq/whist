import applescript from "applescript"
import range from "lodash.range"

const getNumberOfBrowserWindows = (browser: string) => {
  const script = `
        tell application "${browser}" to get number of windows
    `

  return new Promise((resolve, reject) => {
    applescript.execString(script, (err: any, raw: string) => {
      if (err) reject(err)
      resolve(raw)
    })
  })
}

const getAllBrowserWindows = async (browser: string) => {
  const numberOfWindows = await getNumberOfBrowserWindows(browser)
  let windows = []

  for (const i of range(1, Number(numberOfWindows) + 1)) {
    const script = `
        set titleString to ""

        tell application "${browser}"
            set window_list to every window # get the windows
                
            set the_window to item ${i} of window_list
            set tab_list to every tab in the_window # get the tabs
            
            repeat with the_tab in tab_list # for every tab
                    set the_url to the URL of the_tab # grab the URL
                    set titleString to titleString & the_url & "\n"
            end repeat
        end tell
    `

    const urls = await new Promise((resolve, reject) => {
      applescript.execString(script, (err: any, raw: string) => {
        if (err) reject(err)

        let urls = []

        for (const url of raw.split("\n")) {
          if (url !== "") urls.push(url)
        }

        resolve(urls)
      })
    })

    windows.push({ id: i, urls })
  }

  return windows
}

export { getAllBrowserWindows }
