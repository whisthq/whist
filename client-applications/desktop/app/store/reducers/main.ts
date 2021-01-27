import * as MainAction from "store/actions/pure"
import { DEFAULT } from "store/reducers/states"

import { deepCopyObject } from "shared/utils/general/reducer"

const MainReducer = (
    state = DEFAULT,
    action: {
        type: string
        body: Record<string, any>
    }
) => {
    const stateCopy = deepCopyObject(state)
    switch (action.type) {
        case MainAction.RESET_STATE:
            return DEFAULT
        case MainAction.UPDATE_ADMIN:
            return {
                ...stateCopy,
                admin: Object.assign(stateCopy.admin, action.body),
            }
        case MainAction.UPDATE_AUTH:
            return {
                ...stateCopy,
                auth: Object.assign(stateCopy.auth, action.body),
            }
        case MainAction.UPDATE_CLIENT:
            return {
                ...stateCopy,
                client: Object.assign(stateCopy.client, action.body),
            }
        case MainAction.UPDATE_CONTAINER:
            return {
                ...stateCopy,
                container: Object.assign(stateCopy.container, action.body),
            }
        case MainAction.UPDATE_LOADING:
            return {
                ...stateCopy,
                loading: Object.assign(stateCopy.loading, action.body),
            }
        case MainAction.UPDATE_PAYMENT:
            return {
                ...stateCopy,
                payment: Object.assign(stateCopy.payment, action.body),
            }
        case MainAction.UPDATE_APPS: {
            const { authenticated, disconnected } = action.body
            let apps = Object.assign(stateCopy.apps, action.body)

            if (
                authenticated &&
                apps.connectedApps.indexOf(authenticated) < 0
            ) {
                // Add a newly-authenticated application to the list of connected applications.
                apps.connectedApps.push(authenticated)
            } else if (disconnected) {
                const index = apps.connectedApps.indexOf(disconnected)

                if (index > -1) {
                    // Remove the newly-disconnected application from the list of connected
                    // applications.
                    apps.connectedApps.splice(
                        apps.connectedApps.indexOf(disconnected),
                        1
                    )
                }
            }

            return {
                ...stateCopy,
                apps,
            }
        }
        default:
            return state
    }
}

export default MainReducer
