import { FractalKey } from "shared/enums/input"

export const openExternal = (url: string) => {
    const { shell } = require("electron")
    shell.openExternal(url)
}

export const varOrNull = <T>(variable: T): T | null => {
    return variable || null
}

export const isNullString = (str: string): boolean => {
    return str && str !== "" ? true : false
}
