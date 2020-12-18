import { WebSocketLink } from "@apollo/client/link/ws"
import {
    ApolloClient,
    InMemoryCache,
    HttpLink,
    split,
} from "@apollo/client"
import { config } from "shared/constants/config"
import { getMainDefinition } from "@apollo/client/utilities"

const httpLink = new HttpLink({
    uri: config.url.GRAPHQL_HTTP_URL,
})

const wsLink = new WebSocketLink({
    uri: config.url.GRAPHQL_WS_URL,
    options: {
        reconnect: true,
    },
})

const splitLink = split(
    ({ query }) => {
        const definition = getMainDefinition(query)
        return (
            definition.kind === "OperationDefinition" &&
            definition.operation === "subscription"
        )
    },
    wsLink,
    httpLink
)

const apolloClient = new ApolloClient({
    link: splitLink,
    cache: new InMemoryCache(),
})

export default apolloClient