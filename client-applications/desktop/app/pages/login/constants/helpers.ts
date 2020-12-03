import { FractalApp } from "shared/types/ui"
import { FractalAppName } from "shared/enums/client"

export const checkActive = (app: FractalApp): boolean => {
    return app.active
}

export const urlToApp = (
    url: string,
    featuredAppData: FractalApp[]
): { app_id: string; url: string } => {
    for (let i = 0; i < featuredAppData.length; i++) {
        if (
            url
                .toLowerCase()
                .includes(
                    featuredAppData[i].app_id.toLowerCase().replace(/\s+/g, "-")
                ) &&
            featuredAppData[i].app_id !== FractalAppName.CHROME
        ) {
            return { app_id: featuredAppData[i].app_id, url: null }
        }
    }
    return { app_id: FractalAppName, url: url }
}

let counter = 0

export const findDPI = () => {
    return findFirstPositive(
        (x: number) => (
            ++counter, matchMedia(`(max-resolution: ${x}dpi)`).matches
        )
    )
}

const findFirstPositive = <T>(fn: (x: number) => T) => {
    let start = 1
    while (fn(start) <= 0) start <<= 1
    return binSearch(fn, start >>> 1, start) | 0
}

const binSearch = <T>(
    fn: (x: number) => T,
    min: number,
    max: number
): number => {
    if (max < min) return -1 // not found

    const mid = (min + max) >>> 1
    if (fn(mid) > 0) {
        if (mid == min || fn(mid - 1) <= 0) {
            return mid
        }
        return binSearch(fn, min, mid - 1)
    }
    return binSearch(fn, mid + 1, max)
}
