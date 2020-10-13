export const INSERT_WAITLIST = "INSERT_WAITLIST"
export const UPDATE_WAITLIST = "UPDATE_WAITLIST"
export const UPDATE_USER = "UPDATE_USER"
export const DELETE_USER = "DELETE_USER"
export const UPDATE_UNSORTED_LEADERBOARD = "UPDATE_UNSORTED_LEADERBOARD"
export const UPDATE_CLICKS = "UPDATE_CLICKS"
export const UPDATE_APPLICATION_REDIRECT = "UPDATE_APPLICATION_REDIRECt"

export function insertWaitlistAction(
    user_id: string,
    name: string,
    points: number,
    referralCode: string
) {
    return {
        type: INSERT_WAITLIST,
        user_id,
        name,
        points,
        referralCode,
    }
}

export function updateWaitlistAction(waitlist: any) {
    return {
        type: UPDATE_WAITLIST,
        waitlist,
    }
}

export function updateUserAction(points: any, ranking: number) {
    return {
        type: UPDATE_USER,
        points,
        ranking,
    }
}

export function deleteUserAction() {
    return {
        type: DELETE_USER,
    }
}

export function updateClicks(clicks: number) {
    return {
        type: UPDATE_CLICKS,
        clicks,
    }
}

export function updateApplicationRedirect(redirect: boolean) {
    return {
        type: UPDATE_APPLICATION_REDIRECT,
        redirect,
    }
}
