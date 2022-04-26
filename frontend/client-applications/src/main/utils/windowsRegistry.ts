const win =
  process.platform === "win32" ? require("windows-registry-napi") : undefined

export const windowsKey = (registryFolder: string) => {
  /*
  Creates a Windows Registry Key object

  Returns:
   key (Key): Windows Registry Key
*/
  if (process.platform !== "win32")
    throw new Error(`process.platform ${process.platform} is not win32`)

  return new win.Key(
    win.windef.HKEY.HKEY_CURRENT_USER,
    registryFolder,
    win.windef.KEY_ACCESS.KEY_ALL_ACCESS
  )
}
