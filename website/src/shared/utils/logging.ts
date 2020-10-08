export const debugLog = (callback: any) => {
    console.log(process.env.NODE_ENV)
    if (process.env.NODE_ENV === "development") {
        console.log(callback)
    }
}
