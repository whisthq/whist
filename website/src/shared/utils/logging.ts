/* eslint-disable no-console */
// meant for debugging
export const debugLog =
  import.meta.env.NODE_ENV === 'development'
    ? console.log
    : (callback: any) => {}
