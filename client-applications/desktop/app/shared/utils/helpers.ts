export const openExternal = (url: string) => {
    const { shell } = require("electron")
    shell.openExternal(url)
}

export const varOrNull = <T>(variable: T): T | null => {
    return variable || null
}

export const isNullString = (str: string): boolean => {
    return !!(str && str !== "")
}

export const searchArrayByKey = (
    arr: Object[],
    key: string,
    target: string | number | boolean
): Object | null => {
    for (let i = 0; i < arr.length; i += 1) {
        let element = arr[i]
        if (element[key] === target) {
            return element
        }
    }
    return null
}
