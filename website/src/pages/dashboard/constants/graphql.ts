import { gql } from "@apollo/client"

export const GET_USER = gql`
    query GetUser($user_id: String!) {
        users(where: { user_id: { _eq: $user_id } }) {
            can_login
        }
    }
`
