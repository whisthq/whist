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
                          user_id: action.user_id,
                          name: action.name,
                          points: action.points,
                          referralCode: action.referralCode,
                      }
                    : {
                          user_id: action.user_id,
                          name: action.name,
                          points: action.points,
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
                    googleAuthEmail: action.user_id,
                    points: action.points,
                },
            }
        case LoginAction.LOGOUT:
            return {
                ...state,
                loggedIn: false,
                user: {
                    user_id: null,
                    name: null,
                    referrals: 0,
                    points: 0,
                    ranking: 0,
                },
            }
        case WaitlistAction.UPDATE_CLICKS:
            return {
                ...state,
                clicks: {
                    number: action.clicks,
                    lastClicked: new Date().getTime() / 1000,
                },
            }
        case WaitlistAction.UPDATE_APPLICATION_REDIRECT:
            return {
                ...state,
                applicationRedirect: action.redirect,
            }
        default:
            return state
    }
}
