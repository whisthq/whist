/* eslint-disable no-console */
// meant for debugging
export const debugLog =
  import.meta.env.MODE === 'development'
    ? console.log
    : (callback: any) => {}
