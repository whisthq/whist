import { Console } from "node:console"
import { Transform } from "node:stream"

const simpleTable = (input: any) => {
  // @see https://stackoverflow.com/a/67859384
  const ts = new Transform({
    transform(chunk, _, cb) {
      cb(null, chunk)
    },
  })
  const logger = new Console({ stdout: ts, colorMode: true })
  logger.table(input)
  const table = ((ts.read() as string) || "").toString()
  return table
    .split(/[\r\n]+/)
    .map((r) =>
      r
        .replace(/[^┬]*┬/, "┌")
        .replace(/^├─*┼/, "├")
        .replace(/│[^│]*/, "")
        .replace(/^└─*┴/, "└")
        .replace(/'/g, " ")
    )
    .join("\n")
}

export const printOutput = (
  data: any,
  message: string,
  live: boolean = false
) => {
  if (live) console.clear()
  console.log(message)
  console.log(simpleTable(data))
  if (live) console.log(`Last updated: ${new Date().toString()}`)
}
