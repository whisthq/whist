import { DEFAULT, Task, Container } from "store/reducers/container/default"
import * as ContainerAction from "store/actions/container/pure"
import { deepCopyObject } from "shared/utils/general/reducer"

export default function (
    state = DEFAULT,
    action: {
        body: Task | Container
        type: string
    }
) {
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
        default:
            return state
    }
}
