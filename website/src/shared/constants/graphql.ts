import { gql } from "@apollo/client"

export const UPDATE_NAME = gql`
    mutation UpdateName($userID: String, $name: String) {
        update_users(
            where: { user_id: { _eq: $userID } }
            _set: { name: $name }
        ) {
            affected_rows
        }
    }
`
export const UPDATE_ACCESS_TOKEN = gql`
    mutation UpdateAccessToken($loginToken: String!, $accessToken: String) {
        update_tokens(
            where: { login_token: { _eq: $loginToken } }
            _set: { access_token: $accessToken }
        ) {
            affected_rows
        }
    }
`
