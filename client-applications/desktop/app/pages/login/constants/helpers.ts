import { FractalApp } from "shared/types/ui"

export const checkActive = (app: FractalApp): boolean => {
    return app.active
}

export const urlToApp = (
    url: string,
    featuredAppData: FractalApp[]
): { app_id: string; url: string } => {
    for (var i = 0; i < featuredAppData.length; i++) {
        if (
            url
                .toLowerCase()
                .includes(
                    featuredAppData[i].app_id.toLowerCase().replace(/\s+/g, "-")
                ) &&
            featuredAppData[i].app_id !== "Google Chrome"
        ) {
            return { app_id: featuredAppData[i].app_id, url: null }
        }
    }
    return { app_id: "Google Chrome", url: url }
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
    while (0 >= fn(start)) start <<= 1
    return binSearch(fn, start >>> 1, start) | 0
}

const binSearch = <T>(
    fn: (x: number) => T,
    min: number,
    max: number
): number => {
    if (max < min) return -1 // not found

    let mid = (min + max) >>> 1
    if (0 < fn(mid)) {
        if (mid == min || 0 >= fn(mid - 1)) {
            return mid
        }
        return binSearch(fn, min, mid - 1)
    }
    return binSearch(fn, mid + 1, max)
}
