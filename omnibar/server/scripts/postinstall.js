// Build core-ts and electron-notarize after the normal install procedures.
const execCommand = require("./execCommand").execCommand;
const path = require("path");

const postInstall = (_env, ..._args) => {
  console.log("Building `@whist/shared`...");
  execCommand("yarn install", path.join("node_modules", "@whist/shared"));
  execCommand("yarn build", path.join("node_modules", "@whist/shared"));
  console.log("Success building `@whist/shared`!");

  // Recompile node native modules
  console.log("Recompiling node modules for your computer...");
  execCommand("electron-builder install-app-deps");
};

module.exports = postInstall;

if (require.main === module) {
  postInstall();
}
