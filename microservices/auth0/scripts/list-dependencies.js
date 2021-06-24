// List dependencies for a bundled hook
// Usage: `yarn list-dependencies ./dist/[hook-name]`

const fs = require("fs");

const path = process.argv[2];
const data = fs.readFileSync(path, "utf8");

const re = /require\("(.*?)"\)/g;
const matches = new Set();
let match;
while ((match = re.exec(data))) {
  matches.add(match[1]);
}
matches.forEach((match) => console.log(match));
