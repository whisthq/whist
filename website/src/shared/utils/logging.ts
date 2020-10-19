export const debugLog =
    process.env.NODE_ENV === "development" ? console.log : (callback: any) => {}
