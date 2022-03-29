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

execCommand("pushd node_modules/@whist/core-ts")
execCommand("yarn install")
execCommand("yarn build")
execCommand("popd")
execCommand("yarn tailwind")
execCommand("yarn assets:pull")
