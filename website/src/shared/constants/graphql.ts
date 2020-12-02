import { gql } from "@apollo/client"

export const UPDATE_WAITLIST_REFERRALS = gql`
    mutation UpdateWaitlistReferrals(
        $userID: String
        $points: Int
        $referrals: Int
    ) {
        update_waitlist(
            where: { user_id: { _eq: $userID } }
            _set: { points: $points, referrals: $referrals }
        ) {
            affected_rows
        }
    }
`

export const UPDATE_WAITLIST_AUTH_EMAIL = gql`
    mutation UpdateWaitlistAuthEmail($userID: String, $authEmail: String) {
        update_waitlist(
            where: { user_id: { _eq: $userID } }
            _set: { auth_email: $authEmail }
        ) {
            affected_rows
        }
    }
`
