/* scripts/build-tailwind.js
 *
 * This builds our Tailwind CSS configuration.
 *
 * This should be invoked via:
 *   $ yarn tailwind
 * from your command line.
 *
 */

const { execCommand } = require("./execCommand")

execCommand("npx tailwindcss build -o public/css/tailwind.css", ".")
