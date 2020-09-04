export const INSERT_WAITLIST = "INSERT_WAITLIST";
export const UPDATE_WAITLIST = "UPDATE_WAITLIST";

export function insertWaitlistAction(email: string, name: string) {
    return {
        type: INSERT_WAITLIST,
        email,
        name,
    };
}

export function updateWaitlistAction(waitlist) {
    return {
        type: UPDATE_WAITLIST,
        waitlist,
    };
}
