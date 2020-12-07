import { FractalApp } from "shared/types/ui"
import { FractalAppName } from "shared/types/client"

export const checkActive = (app: FractalApp): boolean => {
    return app.active
}

export const urlToApp = (
    url: string,
    featuredAppData: FractalApp[]
): { appID: string; url: string } => {
    for (let i = 0; i < featuredAppData.length; i += 1) {
        if (
            url
                .toLowerCase()
                .includes(
                    featuredAppData[i].app_id.toLowerCase().replace(/\s+/g, "-")
                ) &&
            featuredAppData[i].app_id !== FractalAppName.CHROME
        ) {
            return { appID: featuredAppData[i].app_id, url: null }
        }
    }
    return { appID: FractalAppName.CHROME, url: url }
}

const binSearch = (
    fn: (x: number) => boolean,
    min: number,
    max: number
): number => {
    if (max < min) return -1 // not found

    const mid = (min + max) / 2
    if (fn(mid)) {
        if (mid === min || !fn(mid - 1)) {
            return mid
        }
        return binSearch(fn, min, mid - 1)
    }
    return binSearch(fn, mid + 1, max)
}

const findFirstMatch = (fn: (x: number) => boolean) => {
    let start = 1
    while (!fn(start)) start *= 2
    return binSearch(fn, start / 2, start)
}

export const findDPI = () => {
    return findFirstMatch(
        (x: number) => matchMedia(`(max-resolution: ${x}dpi)`).matches
    )
}
