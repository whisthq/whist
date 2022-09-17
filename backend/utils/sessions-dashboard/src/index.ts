import { DeployEnvironment } from "./config"
import { retrieveSessions } from "./retrieve"
import { getUser } from "./users"
import { Console } from "node:console"
import { Transform } from "node:stream"

const cleanTable = (input: any) => {
  // @see https://stackoverflow.com/a/67859384
  const ts = new Transform({
    transform(chunk, _, cb) {
      cb(null, chunk)
    },
  })
  const logger = new Console({ stdout: ts })
  logger.table(input)
  const table = (ts.read() || "").toString()
  let result = ""
  for (let row of table.split(/[\r\n]+/)) {
    let r = row.replace(/[^┬]*┬/, "┌")
    r = r.replace(/^├─*┼/, "├")
    r = r.replace(/│[^│]*/, "")
    r = r.replace(/^└─*┴/, "└")
    r = r.replace(/'/g, " ")
    result += `${r}\n`
  }
  console.log(result)
}

retrieveSessions().then(async (sessions) => {
  if (!sessions) return

  const data = await Promise.all(
    sessions.map(async (s) => {
      return {
        ...s,
        user: await getUser(s.user_id),
      }
    })
  )

  console.log(`Active sessions for environment "${DeployEnvironment}":`)
  const table = data.map((s) => {
    return {
      name: s.user.name,
      email: s.user.email,
      commit: s.instance.client_sha.substring(0, 8),
      mandelbox: s.id,
      instance: s.instance.id,
      ip: s.instance.ip_addr.split("/")[0],
      region: s.instance.region,
    }
  })
  cleanTable(table)
})
