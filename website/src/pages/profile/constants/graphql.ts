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
