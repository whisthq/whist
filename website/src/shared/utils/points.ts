import { db } from "shared/utils/firebase"

export const INITIAL_POINTS = 50
export const SIGNUP_POINTS = 100
export const REFERRAL_POINTS = 300

// export function sortLeaderboard(leaderboard: any) {
//     var leaderboard_as_list = []
//     for (var email in leaderboard) {
//         leaderboard_as_list.push(leaderboard[email])
//     }
//     leaderboard_as_list = leaderboard_as_list.sort(function (a: any, b: any) {
//         if (!a.points) return 1
//         if (!b.points) return 1
//         if (a.points < b.points) return 1
//         if (a.points > b.points) return -1
//         if (a.points === b.points) {
//             if (a.email < b.email) return 1
//             if (a.email > b.email) return -1
//         }
//         return 1
//     })
//     return leaderboard_as_list
// }

// export async function getSortedLeaderboard(document?: any) {
//     const leaderboard = await getUnsortedLeaderboard(document ? document : null)
//     if (leaderboard) {
//         const sortedLeaderboard = sortLeaderboard(leaderboard)
//         return sortedLeaderboard
//     }
//     return []
// }

// export async function getUnsortedLeaderboard(document?: any) {
//     if (!document) {
//         const points = await db.collection("metadata").doc("waitlist").get()
//         document = points.data()
//     }
//     if (document) {
//         var leaderboard = document.leaderboard
//         return leaderboard
//     }
//     return null
// }

// function checkName(name: string, waitlist: any) {
//     for (var i = 0; i < waitlist.length; i++) {
//         if (waitlist[i].name === name) {
//             console.log(waitlist[i])
//             return [i, true]
//         }
//     }
//     return [0, false]
// }
