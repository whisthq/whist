/*
  Updates the local tenant.yaml file to match the current Auth0 configuration. This is necessary
  so that tenant.yaml matches any modifications made in the Auth0 UI, so that none of those modifications
  are overwritten on a new code push

  To be called via:
  yarn dump:[env]
*/

const { dump } = require("auth0-deploy-cli")
const { getConfig } = require("./config")

const env = process.argv[2]

dump({
  output_folder: ".",
  format: "yaml",
  config: getConfig(env),
})
  .then(() => console.log(`Successfully dumped from Auth0 ${env} tenant!`))
  .catch((err) =>
    console.error(`Failed to dump from Auth0 ${env} tenant. Error: ${err}`)
  )
