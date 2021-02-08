import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiGet, apiPost } from "shared/utils/general/api"

import { FractalAPI, FractalHTTPCode } from "shared/types/api"
import * as ContainerAction from "store/actions/container/sideEffects"
import { updateUser } from "store/actions/auth/pure"
import { FractalTaskStatus } from "shared/types/containers"
import { updateTask, updateContainer } from "store/actions/container/pure"
import { DEFAULT } from "store/reducers/auth/default"
import { FractalRoute } from "shared/types/navigation"
import { history } from "store/history"
import { deepCopyObject } from "shared/utils/general/reducer"
import { setAWSRegion } from "shared/utils/files/aws"
import { findDPI } from "shared/utils/general/dpi"
import { allowedRegions } from "shared/types/aws"
import { FractalLogger } from "shared/utils/general/logging"
import { endStream } from "shared/utils/files/exec"

import { DEFAULT as ContainerDefault } from "store/reducers/container/default"

function* createContainer() {
    /*
    Description:
        Sends POST request to create container with the correct region, DPI, etc.

    Arguments: none
    */
    const logger = new FractalLogger()

    const state = yield select()

    const userID = state.AuthReducer.user.userID
    const accessToken = state.AuthReducer.user.accessToken

    // Get region
    let region = allowedRegions[0]
    try {
        region = yield call(setAWSRegion, accessToken)
        logger.logInfo(`Fetch AWS region found region ${region}`, userID)
    } catch (err) {
        logger.logError(
            `Fetch AWS region errored with ${err}, using default region ${region}`,
            userID
        )
    }

    // Get client DPI
    const dpi = findDPI()

    // Send container assign request
    const { json, success, response } = yield call(
        apiPost,
        FractalAPI.CONTAINER.ASSIGN,
        {
            username: userID,
            region: region,
            app: "Google Chrome",
            dpi: dpi,
        },
        accessToken
    )

    // Get the Celery task ID
    if (success && json.ID) {
        yield put(updateTask({ taskID: json.ID }))
    } else {        
        if (response.status === FractalHTTPCode.PAYMENT_REQUIRED) {
            yield call(history.push, FractalRoute.PAYMENT)
        } else {
            yield put(updateUser(deepCopyObject(DEFAULT.user)))
            yield call(history.push, FractalRoute.LOGIN)
        }

        yield put(updateTask(ContainerDefault.task))
    }
}

function* getContainerInfo(action: { taskID: string }) {
    /*
    Description:
        Sends GET request to retrieve Celery task status (for container creation)

    Arguments: 
        action (Record): Redux action with Celery task ID
    */

    const state = yield select()
    const accessToken = state.AuthReducer.user.accessToken

    const { json, success } = yield call(
        apiGet,
        `/status/${action.taskID}`,
        accessToken
    )

    if (success && json && json.output) {
        yield put(
            updateTask({
                status: FractalTaskStatus.SUCCESS,
            })
        )

        yield put(
            updateContainer({
                containerID: json.output.container_id,
                cluster: json.output.cluster,
                port32262: json.output.port_32262,
                port32263: json.output.port_32263,
                port32273: json.output.port_32273,
                location: json.output.location,
                publicIP: json.output.ip,
                secretKey: json.output.secret_key,
            })
        )
    } else {
        yield put(
            updateTask({
                status: FractalTaskStatus.FAILURE,
            })
        )
    }
}

export default function* containerSaga() {
    yield all([
        takeEvery(ContainerAction.CREATE_CONTAINER, createContainer),
        takeEvery(ContainerAction.GET_CONTAINER_INFO, getContainerInfo),
    ])
}
