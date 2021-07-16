import { createBrowserHistory, createMemoryHistory } from 'history'

const chooseHistory = () => {
  /* A helper function to create a history object
   *
   * In certain contexts, like e2e testing, we need to call website helper functions
   * to peform tasks like creating a test user. This causes the "history" library to
   * be imported. If we just defaulted to createBrowserHistory(), then the import would
   * fail in Node.js contexts like Puppeteer testing. We need to switch the history
   * functoin based on the execution environment.
   */

  const isBrowser =
    typeof window !== 'undefined' && typeof window.document !== 'undefined'

  if (isBrowser) return createBrowserHistory()
  return createMemoryHistory()
}

export default chooseHistory()
