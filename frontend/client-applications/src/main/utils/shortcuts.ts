import { globalShortcut } from "electron"

const createGlobalShortcut = (shortcut: string, fn: () => void) => {
  globalShortcut.register(shortcut, () => {
    fn()
  })
}

export { createGlobalShortcut }
