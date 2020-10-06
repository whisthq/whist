import * as MainAction from 'store/actions/pure'
import { DEFAULT } from 'store/reducers/states'

export default function MainReducer(state = DEFAULT, action: any) {
    switch (action.type) {
        case MainAction.RESET_STATE:
            return DEFAULT
        case MainAction.UPDATE_AUTH:
            return {
                ...state,
                auth: Object.assign(state.auth, action.body),
            }
        case MainAction.UPDATE_CLIENT:
            return {
                ...state,
                client: Object.assign(state.client, action.body),
            }
        case MainAction.UPDATE_CONTAINER:
            return {
                ...state,
                container: Object.assign(state.container, action.body),
            }
        case MainAction.UPDATE_LOADING:
            return {
                ...state,
                loading: Object.assign(state.loading, action.body),
            }
        case MainAction.UPDATE_PAYMENT:
            return {
                ...state,
                payment: Object.assign(state.payment, action.body),
            }
        default:
            return state
    }
}
