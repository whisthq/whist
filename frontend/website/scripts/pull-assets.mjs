/* scripts/lint.js
 *
 * This script uses the shiny new Notion API
 * https://developers.notion.com/reference
 * to load our assets (i.e. team photos) into the website
 *
 * This should be invoked via:
 *   $ yarn assets:pull
 * from your command line.
 *
 */
import { fetchCompanyInfo } from "./notion-helpers.js"

// Queries Notion API, saves images to S3, and caches results
console.log("Pulling assets from Notion database...")

// Pull investor and team data from Notion
await fetchCompanyInfo()

console.log("Assets loaded successfully")
