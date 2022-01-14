import { globalShortcut, Menu } from "electron"

const createGlobalShortcut = (shortcut: string, fn: () => void) => {
  globalShortcut.register(shortcut, () => {
    fn()
  })
}

export { createGlobalShortcut }
