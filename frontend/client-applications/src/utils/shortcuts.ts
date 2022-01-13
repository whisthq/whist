import { globalShortcut } from "electron"
import EventEmitter from "events"

// Electron shortcuts take callbacks instead of emitting events, so we need to make our own EventEmitter
const shortcutEmitter = new EventEmitter()

const createGlobalShortcut = (shortcut: string, fn: () => void) => {
  globalShortcut.register(shortcut, () => {
    fn()
  })
}

export { shortcutEmitter, createGlobalShortcut }
