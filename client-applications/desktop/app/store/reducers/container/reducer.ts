import {
    DEFAULT,
    Task,
    Container,
    HostService,
} from "store/reducers/container/default"
import * as ContainerAction from "store/actions/container/pure"
import { deepCopyObject } from "shared/utils/general/reducer"

export default (
    state = DEFAULT,
    action: {
        body: Task | Container | HostService
        type: string
    }
) => {
    /*
        Description:
            Reducer for container actions
        Arguments:
            body (Task | Container | HostService ): Action body 
            type (string): Action type
    */

    const stateCopy = deepCopyObject(state)
    switch (action.type) {
        case ContainerAction.UPDATE_TASK:
            return {
                ...stateCopy,
                task: Object.assign(stateCopy.task, action.body),
            }
        case ContainerAction.UPDATE_CONTAINER:
            return {
                ...stateCopy,
                container: Object.assign(stateCopy.container, action.body),
            }
        case ContainerAction.UPDATE_HOST_SERVICE:
            return {
                ...stateCopy,
                hostService: Object.assign(stateCopy.hostService, action.body),
            }
        default:
            return state
    }
}
