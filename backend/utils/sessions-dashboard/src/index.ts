import { retrieveSessions } from "./retrieve"
import { getUser } from "./users"

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

  data.map((s) => {
    console.log(
      `${s.user.name} (${s.user.email}) is running commit ${
        s.instance.client_sha
      } of ${s.app.toLowerCase()} on mandelbox ${s.id} on instance ${
        s.instance.id
      } at ${s.instance.ip_addr} in ${s.instance.region}.`
    )
  })
  // console.dir(data, { depth: null })
})
