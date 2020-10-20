export const SEND_REFERRAL_EMAIL = "SEND_REFERRAL_EMAIL"
export const GIVE_SECRET_POINTS = "GIVE_SECRET_POINTS"
export const CREATE_SECRET_POINTS = "CREATE_SECRET_POINTS"

export function sendReferralEmail(recipient: string) {
    return {
        type: SEND_REFERRAL_EMAIL,
        recipient,
    }
}

export function giveSecretPointsAction(secretPointsName: string) {
    return {
        type: GIVE_SECRET_POINTS,
        secretPointsName,
    }
}

export function createSecretPointsAction() {
    return {
        type: CREATE_SECRET_POINTS,
    }
}