export const debugLog = <T>(callback: T) => {
    console.log(process.env.NODE_ENV)
    if (process.env.NODE_ENV === "development") {
        console.log(callback)
    }
}
