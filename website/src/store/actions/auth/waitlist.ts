export const INSERT_WAITLIST = "INSERT_WAITLIST";

export function insertWaitlistAction(email: string, name: string) {
    return {
        type: INSERT_WAITLIST,
        email,
        name
    }
}