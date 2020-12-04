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
    arr: Record<string, any>[],
    key: string,
    target: string | number | boolean
): {
    index: number
    value: Record<string, any> | null
} => {
    for (let i = 0; i < arr.length; i += 1) {
        let value = arr[i]
        if (!value.hasOwnProperty(key)) {
            throw ReferenceError
        }
        if (value[key] === target) {
            return { value: value, index: i }
        }
    }
    return { value: null, index: -1 }
}
