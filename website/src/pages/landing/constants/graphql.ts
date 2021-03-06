import { gql } from "@apollo/client"

export const INSERT_WAITLIST = gql`
    mutation InsertWaitlist(
        $userID: String!
        $name: String!
        $points: Int!
        $referralCode: String!
        $referrals: Int!
        $country: String!
    ) {
        insert_waitlist(
            objects: {
                name: $name
                points: $points
                real_user: true
                referral_code: $referralCode
                referrals: $referrals
                user_id: $userID
                country: $country
            }
            on_conflict: { constraint: waitlist_pkey, update_columns: name }
        ) {
            affected_rows
            returning {
                name
                points
                referral_code
                referrals
                user_id
                country
            }
        }
    }
`

export const SUBSCRIBE_WAITLIST = gql`
    subscription SubscribeWaitlist {
        waitlist(
            order_by: { points: desc }
            where: { on_waitlist: { _eq: true } }
        ) {
            name
            points
            user_id
            referrals
            referral_code
            auth_email
        }
    }
`
