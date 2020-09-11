export const INSERT_WAITLIST = "INSERT_WAITLIST";
export const UPDATE_WAITLIST = "UPDATE_WAITLIST";
export const UPDATE_USER = "UPDATE_USER";

export function insertWaitlistAction(
    email: string,
    name: string,
    points: number
) {
    return {
        type: INSERT_WAITLIST,
        email,
        name,
        points,
    };
}

export function updateWaitlistAction(waitlist) {
    return {
        type: UPDATE_WAITLIST,
        waitlist,
    };
}

export function updateUserAction(points, ranking) {
    return {
        type: UPDATE_USER,
        points,
        ranking,
    };
}
