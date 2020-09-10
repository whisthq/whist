export const INSERT_WAITLIST = "INSERT_WAITLIST"
export const UPDATE_WAITLIST = "UPDATE_WAITLIST"
export const UPDATE_USER = "UPDATE_USER"
export const DELETE_USER = "DELETE_USER"
export const UPDATE_UNSORTED_LEADERBOARD = "UPDATE_UNSORTED_LEADERBOARD"
export const UPDATE_CLICKS = "UPDATE_CLICKS"
export const SET_CLOSING_DATE = "SET_CLOSING_DATE"

export function insertWaitlistAction(
    email: string,
    name: string,
    points: number,
    referralCode: string,
    ranking: number,
    closingDate: number
) {
    return {
        type: INSERT_WAITLIST,
        email,
        name,
        points,
        referralCode,
        ranking,
        closingDate,
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

export function updateUnsortedLeaderboardAction(unsortedLeaderboard: any) {
    return {
        type: UPDATE_UNSORTED_LEADERBOARD,
        unsortedLeaderboard,
    }
}

export function updateClicks(clicks: number) {
    return {
        type: UPDATE_CLICKS,
        clicks,
    }
}

export function setClosingDateAction(closingDate: any) {
    return {
        type: SET_CLOSING_DATE,
        closingDate,
    }
}
