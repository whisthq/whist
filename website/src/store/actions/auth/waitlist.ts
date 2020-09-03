export const INSERT_WAITLIST = "INSERT_WAITLIST";

export function insertWaitlistAction(email: string, name: string) {
    console.log("ACTION TRIGGERED")
    console.log(email)
    console.log(name)
    return {
        type: INSERT_WAITLIST,
        email,
        name
    }
}