/* scripts/lint.js
 *
 * This script triggers lint checking and/or fixing for all of
 * our source files, using ESLint.
 *
 * This should be invoked via:
 *   $ yarn lint:check
 * or:
 *   $ yarn lint:fix
 * from your command line.
 *
 */

const { execute } = require("../node_modules/eslint/lib/cli")
const execCommand = require("./execCommand").execCommand

execCommand(`yarn prettier ${process.argv.slice(2, 3)} .`, ".")
// The first two arguments here are simply placeholders, as ESLint CLI expects to be called in this way
execute([
  "yarn",
  "eslint",
  ...process.argv.slice(3),
  "--max-warnings=0",
  "./src/**/*.{js,jsx,ts,tsx}",
]).then((ret) => process.exit(ret))
