/* scripts/postInstall.js
 *
 * This builds the Whist website after the normal install procedures.
 *
 * This should be invoked via:
 *   $ yarn postinstall
 * from your command line.
 *
 */

const { execCommand } = require("./execCommand")

execCommand("yarn assets:pull", ".")
execCommand("yarn tailwind", ".")
execCommand("yarn build", ".")
