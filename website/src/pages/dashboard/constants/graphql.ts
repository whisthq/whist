import { gql } from "@apollo/client"

export const GET_USER = gql`
    query GetUser($user_id: String!) {
        users(where: { user_id: { _eq: $user_id } }) {
            can_login
        }
    }
`

export const GET_WAITLIST_USER_FROM_TOKEN = gql`
    query GetWaitlistUserFromToken($waitlist_access_token: String!) {
        waitlist(
            where: { waitlist_access_token: { _eq: $waitlist_access_token } }
        ) {
            user_id
        }
    }
`

export const NULLIFY_WAITLIST_TOKEN = gql`
    mutation NullifyWaitlistToken($user_id: String!) {
        update_waitlist(
            where: { user_id: { _eq: $user_id } }
            _set: { waitlist_access_token: null }
        ) {
            affected_rows
        }
    }
`

export const UPDATE_USER_CAN_LOGIN = gql`
    mutation UpdateUserCanLogin($user_id: String!) {
        update_users(
            where: { user_id: { _eq: $user_id } }
            _set: { can_login: true }
        ) {
            affected_rows
        }
    }
`
