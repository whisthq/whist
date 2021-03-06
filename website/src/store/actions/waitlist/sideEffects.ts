export const SEND_REFERRAL_EMAIL = "SEND_REFERRAL_EMAIL"

export function sendReferralEmail(recipient: string) {
    return {
        type: SEND_REFERRAL_EMAIL,
        recipient,
    }
}
