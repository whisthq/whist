import { gql } from "@apollo/client"

export const SUBSCRIBE_USER = gql`
    subscription SubscribeUser($userID: String!) {
        users(where: { user_id: { _eq: $userID } }) {
            can_login
        }
    }
`

export const GET_WAITLIST_USER_FROM_TOKEN = gql`
    query GetWaitlistUserFromToken($waitlistAccessToken: String!) {
        waitlist(
            where: { waitlist_access_token: { _eq: $waitlistAccessToken } }
        ) {
            user_id
        }
    }
`

export const NULLIFY_WAITLIST_TOKEN = gql`
    mutation NullifyWaitlistToken($userID: String!) {
        update_waitlist(
            where: { user_id: { _eq: $userID } }
            _set: { waitlist_access_token: null }
        ) {
            affected_rows
        }
    }
`

export const UPDATE_USER_CAN_LOGIN = gql`
    mutation UpdateUserCanLogin($userID: String!) {
        update_users(
            where: { user_id: { _eq: $userID } }
            _set: { can_login: true }
        ) {
            affected_rows
        }
    }
`
