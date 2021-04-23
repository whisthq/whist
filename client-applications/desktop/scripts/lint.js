/* scripts/lint.js
 *
 * This script triggers lint checking and/or fixing for all of
 * our source files, using ESLint and Prettier.
 *
 * This should be invoked via:
 *   $ yarn lint:check
 * or:
 *   $ yarn lint:fix
 * from your command line.
 *
 * The format in package.json is:
 *  $ node scripts/lint.js <prettier flag> <eslint flags>
 */

const { execute } = require('../node_modules/eslint/lib/cli')
const execCommand = require('./execCommand').execCommand

// The third argument is the flag for prettier, either --check or --write
// execCommand(`yarn prettier ${process.argv.slice(2, 3)} .`, '.')

// The first two arguments here are simply placeholders, as ESLint CLI expects to be called in this way
// wWe start on the 4th argument as the first 3 are reserved for placeholders/prettier
execute([
    'yarn',
    'eslint',
    ...process.argv.slice(3),
    '--max-warnings=0',
    './src/**/*.{js,jsx,ts,tsx}',
]).then((ret) => process.exit(ret))
