export const UPDATE_WAITLIST_USER = "UPDATE_WAITLIST_USER"
export const UPDATE_WAITLIST_DATA = "UPDATE_WAITLIST_DATA"
export const UPDATE_NAVIGATION = "UPDATE_NAVIGATION"

export const RESET_WAITLIST_USER = "RESET_WAITLIST_USER"

export function updateWaitlistUser(body: {
    user_id?: string
    name?: string
    referrals?: number
    points?: number
    ranking?: number
    referralCode?: string
    authEmail?: string
}) {
    return {
        type: UPDATE_WAITLIST_USER,
        body,
    }
}

export function updateWaitlistData(body: {
    waitlist?: any[]
    closingDate?: any
}) {
    return {
        type: UPDATE_WAITLIST_DATA,
        body,
    }
}

export function updateNavigation(body: { applicationRedirect?: boolean }) {
    return {
        type: UPDATE_NAVIGATION,
        body,
    }
}

export function resetWaitlistUser() {
    return {
        type: RESET_WAITLIST_USER,
    }
}
