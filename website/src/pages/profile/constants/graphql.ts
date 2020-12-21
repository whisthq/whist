import { gql } from "@apollo/client"

export const UPDATE_NAME = gql`
    mutation UpdateName($user_id: String, $name: String) {
        update_users(
            where: { user_id: { _eq: $user_id } }
            _set: { name: $name }
        ) {
            affected_rows
        }
    }
`
