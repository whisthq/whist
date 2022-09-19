import { retrieveSessions } from "./sessions"
import { getUser } from "./users"

export const generateData = async () => {
  const sessions = await retrieveSessions()
  if (!sessions) return

  const data = await Promise.all(
    sessions.map(async (s) => {
      return {
        ...s,
        user: await getUser(s.user_id),
      }
    })
  )

  return data.map((s) => {
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
}
