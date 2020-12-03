/* eslint-disable no-console */
export const debugLog = <T>(callback: T) => {
    console.log(process.env.NODE_ENV)
    if (process.env.NODE_ENV === "development") {
        console.log(callback)
    }
}
/* eslint-enable no-console */

// default export until we have multiple exports
export default debugLog
