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
export const INSERT_INVITE = gql`
    mutation InsertInvite($userID: String!) {
        insert_invites(
            objects: { typeform_submitted: true, user_id: $userID }
        ) {
            affected_rows
        }
    }
`

export const UPDATE_INVITE = gql`
    mutation UpdateInvite($userID: String!, $typeformSubmitted: Boolean!) {
        update_invites(
            where: { user_id: { _eq: $userID } }
            _set: { typeform_submitted: $typeformSubmitted }
        ) {
            affected_rows
        }
    }
`

export const SUBSCRIBE_INVITE = gql`
    subscription SubscribeInvite($userID: String!) {
        invites(where: { user_id: { _eq: $userID } }) {
            access_granted
            invites_remaining
            typeform_submitted
            user_id
        }
    }
`