const fs = require("fs")

fs.writeFileSync(".env", `PRODUCTION_ENV=${process.argv[2]}\n`)
console.log("Wrote to `.env.`")