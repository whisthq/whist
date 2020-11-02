export const SUBMIT_PURCHASE_FEEDBACK = "SUBMIT_PURCHASE_FEEDBACK"

export function submitFeedback(feedback: string) {
    return {
        type: SUBMIT_PURCHASE_FEEDBACK,
        feedback,
    }
}
