/* eslint-disable no-console */
export const debugLog =
    import.meta.env.NODE_ENV === "development"
        ? console.log
        : (callback: any) => {}
