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

export const INSERT_INVITE = gql`
    mutation InsertInvite(
        $userID: String!
        $accessGranted: Boolean!
        $typeformSubmitted: Boolean!
    ) {
        insert_invites(
            objects: {
                access_granted: $accessGranted
                typeform_submitted: $typeformSubmitted
                user_id: $userID
            }
        ) {
            affected_rows
        }
    }
`

export const DELETE_INVITE = gql`
    mutation DeleteInvite($userID: String!) {
        delete_invites(where: { user_id: { _eq: $userID } }) {
            affected_rows
        }
    }
`
