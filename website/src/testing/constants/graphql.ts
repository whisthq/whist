import { gql } from "@apollo/client"

export const UPDATE_USER = gql`
    mutation UpdateUser(
        $userID: String!
        $stripeCustomerID: String!
        $verified: Boolean!
    ) {
        update_users(
            where: { user_id: { _eq: $userID } }
            _set: { verified: $verified, stripe_customer_id: $stripeCustomerID }
        ) {
            affected_rows
        }
    }
`

export const DELETE_USER = gql`
    mutation DeleteUser($userID: String!) {
        delete_users(where: { user_id: { _eq: $userID } }) {
            affected_rows
        }
    }
`
