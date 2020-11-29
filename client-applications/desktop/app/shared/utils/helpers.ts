export const openExternal = (url: string) => {
    const { shell } = require("electron")
    shell.openExternal(url)
}
