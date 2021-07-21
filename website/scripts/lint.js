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

// The first two arguments here are simply placeholders, as ESLint CLI expects to be called in this way

const lint = (_env, ...args) => {
  execute([
    "yarn",
    "eslint",
    ...args,
    "--max-warnings=0",
    "./src/**/*.{js,jsx,ts,tsx}",
    "./scripts/**/*.js",
    "./public/**/*.{js,ts}",
    "./*.js",
  ]).then((ret) => process.exit(ret))
}

module.exports = lint

if (require.main === module) {
  lint({}, ...process.argv.slice(2))
}
