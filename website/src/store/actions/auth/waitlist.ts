export const INSERT_WAITLIST = "INSERT_WAITLIST"
export const UPDATE_WAITLIST = "UPDATE_WAITLIST"
export const UPDATE_USER = "UPDATE_USER"
export const DELETE_USER = "DELETE_USER"
export const UPDATE_UNSORTED_LEADERBOARD = "UPDATE_UNSORTED_LEADERBOARD"
export const UPDATE_CLICKS = "UPDATE_CLICKS"
export const UPDATE_APPLICATION_REDIRECT = "UPDATE_APPLICATION_REDIRECt"
export const SET_CLOSING_DATE = "SET_CLOSING_DATE"
export const REFER_EMAIL = "REFER_EMAIL"

export function insertWaitlistAction(
    user_id: string,
    name: string,
    points: number,
    referralCode: string
    closingDate: number
) {
    return {
        type: INSERT_WAITLIST,
        user_id,
        name,
        points,
        referralCode,
        closingDate,
    }
}

export function updateWaitlistAction(waitlist: any) {
    return {
        type: UPDATE_WAITLIST,
        waitlist,
    }
}

export function updateUserAction(
    points: any,
    ranking: number,
    referralCode: string
) {
    return {
        type: UPDATE_USER,
        points,
        ranking,
        referralCode,
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

export function setClosingDateAction(closingDate: any) {
    return {
        type: SET_CLOSING_DATE,
        closingDate,
    }
}

export const referEmailAction = (
    email: string,
    name: string,
    code: string,
    recipient: string
) => {
    return {
        type: REFER_EMAIL,
        email,
        name,
        code,
        recipient,
    }
}
