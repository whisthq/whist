const exec = require("child_process").exec;

const esbuildCommand = [
  "esbuild",
  "src/index.ts",
  "--bundle",
  "--platform=node",
  "--outdir=dist",
];
// We minify our output to make this less convenient for snooping users.
if (process.env.NODE_ENV === "production") {
  esbuildCommand.push("--minify");
}

const cmdBuild = esbuildCommand.join(" ");

exec(cmdBuild);
