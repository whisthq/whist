export const UPDATE_WAITLIST_USER = "UPDATE_WAITLIST_USER"
export const UPDATE_CLICKS = "UPDATE_CLICKS"
export const UPDATE_WAITLIST_DATA = "UPDATE_WAITLIST_DATA"
export const UPDATE_NAVIGATION = "UPDATE_NAVIGATION"

export function updateWaitlistUser(body: {
    referrals?: number
    points?: number
    ranking?: number
    referralCode?: string
}) {
    return {
        type: UPDATE_WAITLIST_USER,
        body,
    }
}

export function updateClicks(body: { number?: number; lastClicked?: number }) {
    return {
        type: UPDATE_CLICKS,
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
