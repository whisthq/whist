export const debugLog = (callback: any) => {
    if (process.env.NODE_ENV === "development") {
        console.log(callback)
    }
}
