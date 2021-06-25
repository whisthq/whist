/**
 * Copyright Fractal Computers, Inc. 2021
 * @file history.ts
 * @brief This file contains utility functions for browser history, which is used by the renderer
 * thread to navigate between page routes.
 */

import { createHashHistory, createMemoryHistory } from "history"

const chooseHistory = () => {
  /* A helper function to create a history object
   *
   * In certain contexts, like e2e testing, we need to call website helper functions
   * to peform tasks like creating a test user. This causes the "history" library to
   * be imported. If we just defaulted to createBrowserHistory(), then the import would
   * fail in Node.js contexts like Puppeteer testing. We need to switch the history
   * function based on the execution environment.
   */

  const isBrowser =
    typeof window !== "undefined" && typeof window.document !== "undefined"

  if (isBrowser) return createHashHistory()
  return createMemoryHistory()
}

export default chooseHistory()

export const browserHistory = chooseHistory()

export const goTo = (url: string) => {
  browserHistory.push(url)
}
