import { AUTH_DEFAULT } from "store/reducers/states"

import * as WaitlistAction from "store/actions/auth/waitlist"
import * as LoginAction from "store/actions/auth/login_actions"

export default function (state = AUTH_DEFAULT, action: any) {
    switch (action.type) {
        case WaitlistAction.UPDATE_WAITLIST:
            return {
                ...state,
                waitlist: action.waitlist,
            }
        case WaitlistAction.INSERT_WAITLIST:
            return {
                ...state,
                user: state.user
                    ? {
                          ...state.user,
                          email: action.email,
                          name: action.name,
                          points: action.points,
                          ranking: action.ranking,
                          referralCode: action.referralCode,
                      }
                    : {
                          email: action.email,
                          name: action.name,
                          points: action.points,
                          ranking: action.ranking,
                          referralCode: action.referralCode,
                      },
            }
        case WaitlistAction.UPDATE_USER:
            return {
                ...state,
                user: {
                    ...state.user,
                    points: action.points,
                    ranking: action.ranking,
                },
            }
        case WaitlistAction.DELETE_USER:
            return {
                ...state,
                user: AUTH_DEFAULT.user,
            }
        case LoginAction.GOOGLE_LOGIN:
            return {
                ...state,
                loggedIn: true,
                user: {
                    ...state.user,
                    googleAuthEmail: action.email,
                    points: action.points,
                },
            }
        case LoginAction.LOGOUT:
            return {
                ...state,
                loggedIn: false,
                user: {
                    email: null,
                    name: null,
                    referrals: 0,
                    points: 0,
                    ranking: 0,
                },
            }
        case WaitlistAction.UPDATE_UNSORTED_LEADERBOARD:
            return {
                ...state,
                unsortedLeaderboard: action.unsortedLeaderboard,
            }
        case WaitlistAction.UPDATE_CLICKS:
            return {
                ...state,
                clicks: {
                    number: action.clicks,
                    lastClicked: new Date().getTime() / 1000,
                },
            }
        case WaitlistAction.SET_CLOSING_DATE:
            return {
                ...state,
                closing_date: action.closingDate,
            }
        default:
            return state
    }
}
