# Sessions Dashboard

This tool is a useful way to get information about all running sessions and the associated users.

### Usage

Running `npm run start` will compile and start the script.

The script depends on authenticating with Auth0 and Hasura, and therefore needs some config variables. You can pass these in either using `npm run start --k1=v1 --k2=v2 ...`, or in a `.npmrc` file. For example, to interact with prod we may use:

```npmrc
AUTH0_CLIENT_SECRET = "Client Secret from Auth0 Dashboard"
AUTH0_CLIENT_ID = "Client ID from Auth0 Dashboard"
HASURA_ADMIN_SECRET = "Hasura Secret from Heroku Config Variables"
DEPLOY_ENVIRONMENT = "prod"
```

For the Auth0 client information, I chose the machine-to-machine application entitled `Auth0 Deploy CLI`. This is the client we use for dumping and deploying Auth0 tenants from the YAML configuration file, but the same client suffices for our purposes here.
