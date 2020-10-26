import { gql } from "@apollo/client"

export const UPDATE_WAITLIST = gql`
    mutation UpdateWaitlist($user_id: String, $points: Int, $referrals: Int) {
        update_waitlist(
            where: { user_id: { _eq: $user_id } }
            _set: { points: $points, referrals: $referrals }
        ) {
            affected_rows
        }
    }
`

export const UPDATE_WAITLIST_AUTH_EMAIL = gql`
    mutation UpdateWaitlistAuthEmail($user_id: String, $authEmail: String) {
        update_waitlist(
            where: { user_id: { _eq: $user_id } }
            _set: { auth_email: $authEmail }
        ) {
            affected_rows
        }
    }
`