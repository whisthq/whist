const httpConfig = (method: string) => async (args: {
  body: object
  url: string
  accessToken?: string
}) => {
  const response = await fetch(args.url, {
    method,
    headers: {
      "Content-Type": "application/json",
      ...(args.accessToken !== undefined && {
        Authorization: `Bearer ${args.accessToken}`,
      }),
    },
    body: JSON.stringify(args.body),
  })

  const json = await response.json()

  return {
    status: response.status,
    json,
  }
}

const post = httpConfig("POST")
const put = httpConfig("PUT")

export { post, put }
