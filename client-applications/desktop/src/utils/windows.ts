// const remToPx = (fontSize: number, rem: number) => rem * fontSize
// const toPx = (rem: number) => remToPx(16, rem)
import type { BrowserWindowConstructorOptions } from "electron"

const titleStyle =
    process.platform === "darwin"
        ? { titleBarStyle: "hidden" }
        : { frame: false }

const base = {
    webPreferences: { contextIsolation: true },
    resizable: false,
    titleBarStyle: "hidden",
}

const wXs = { width: 16 * 24 }
const wSm = { width: 16 * 32 }
const wMd = { width: 16 * 40 }
const wLg = { width: 16 * 56 }
const wXl = { width: 16 * 64 }
const w2Xl = { width: 16 * 80 }
const w3Xl = { width: 16 * 96 }

const hXs = { height: 16 * 24 }
const hSm = { height: 16 * 32 }
const hMd = { height: 16 * 40 }
const hLg = { height: 16 * 56 }
const hXl = { height: 16 * 64 }
const h2Xl = { height: 16 * 80 }
const h3Xl = { height: 16 * 96 }

export const windowThinSm = {
    ...base,
    ...wSm,
    ...hMd,
} as BrowserWindowConstructorOptions
