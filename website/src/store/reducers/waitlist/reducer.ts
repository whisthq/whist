import { DEFAULT } from "store/reducers/waitlist/default"

import * as PureAction from "store/actions/waitlist/pure"
import * as SideEffectAction from "store/actions/waitlist/sideEffects"
import * as SharedAction from "store/actions/shared"

import { deep_copy, if_exists_else, if_exists_spread, modified } from "shared/utils/reducerHelpers"
import { SECRET_POINTS } from "shared/utils/points"

const emptySet = new Set()

export default function (state = DEFAULT, action: any) {
    var stateCopy = deep_copy(state)
    switch (action.type) {
        case PureAction.UPDATE_WAITLIST_USER:
            return {
                ...stateCopy,
                user: Object.assign(stateCopy.waitlistUser, action.body),
            }
        case PureAction.UPDATE_CLICKS:
            return {
                ...stateCopy,
                clicks: Object.assign(stateCopy.clicks, action.body),
            }
        case PureAction.UPDATE_WAITLIST_DATA:
            return {
                ...stateCopy,
                waitlistData: Object.assign(
                    stateCopy.waitlistData,
                    action.body
                ),
            }

        case PureAction.UPDATE_NAVIGATION:
            return {
                ...stateCopy,
                navigation: Object.assign(stateCopy.navigation, action.body),
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        case SideEffectAction.CREATE_SECRET_POINTS:
            // console.log('dafuqq')
            return {
                ...state,
                waitlistUser: if_exists_spread(
                    state.waitlistUser, 
                    {
                        secret_points_available : new Set(Object.values(SECRET_POINTS))
                    }
                )
            }
        case SideEffectAction.GIVE_SECRET_POINTS:
            return {
                ...state,
                waitlistUser: if_exists_spread(
                    state.waitlistUser, 
                    {
                        secret_points_available : modified(
                            if_exists_else(state, ["waitlistUser", "secret_points_available"], emptySet), 
                            "delete",
                            [action.secretPointsName], 
                            true // deep copy so redux won't yell at us (might have something to do w/ history)
                        )
                    }
                )
            }
        default:
            return state
    }
}
