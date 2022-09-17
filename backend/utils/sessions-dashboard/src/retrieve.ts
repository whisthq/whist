import fetch from "node-fetch"
import { DeployEnvironment, HasuraAdminSecret } from "./config"

export interface SessionsData {
  app: string
  instance: {
    client_sha: string
    created_at: string
    id: string
    image_id: string
    instance_type: string
    ip_addr: string
    region: string
    updated_at: string
  }
  id: string
  updated_at: string
  session_id: string
  user_id: string
  fetchedAt: string
}

type JSONResponse = {
  data?: {
    whist_mandelboxes: [Omit<SessionsData, "fetchedAt">]
  }
  errors?: Array<{ message: string }>
}

const fetchGraphQL = async (
  operationsDoc: string,
  operationName: string,
  variables: Record<string, any>
) => {
  const result = await fetch(
    `https://whist-${DeployEnvironment}-hasura.herokuapp.com/v1/graphql`,
    {
      method: "POST",
      headers: {
        "content-type": "application/json",
        "x-hasura-admin-secret": HasuraAdminSecret,
      },
      body: JSON.stringify({
        query: operationsDoc,
        variables,
        operationName,
      }),
    }
  )
  const { data, errors }: JSONResponse = (await result.json()) ?? {}
  if (errors) {
    console.error(errors)
    return null
  }
  if (!data) {
    console.error("Data not found")
    return null
  }
  return data.whist_mandelboxes
}

const operation = `
  query RunningSessions {
    whist_mandelboxes(where: {status: {_eq: "RUNNING"}}) {
      app
      instance {
        client_sha
        created_at
        id
        image_id
        instance_type
        ip_addr
        region
        updated_at
      }
      id
      updated_at
      session_id
      user_id
    }
  }
`

export const retrieveSessions = async () => {
  try {
    return await fetchGraphQL(operation, "RunningSessions", {})
  } catch (e) {
    console.error(e)
    return null
  }
}
