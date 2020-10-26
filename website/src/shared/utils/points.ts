export const INITIAL_POINTS = 50
export const SIGNUP_POINTS = 100
export const REFERRAL_POINTS = 300
// this will change every time you reload so technically they can game the system to get the maximal 60*3 = 180 points
// however that it's unlikely everyone will and probably we will get semi-uniform looking leaderboard scores
export const EASTEREGG_POINTS = 50
export const EASTEREGG_RAND = () => Math.floor(Math.random() * 10)

// js sets are bugged
export const SECRET_POINTS = {
    LANDING_NO_GPU_NO_PROBLEM: "LANDING_NO_GPU_NO_PROBLEM",
    ABOUT_HOW_IT_WORKS: "ABOUT_HOW_IT_WORKS",
    FOOTER_SUPER_SECRET: "FOOTER_SUPER_SECRET",
}
