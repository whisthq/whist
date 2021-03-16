import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiGet, apiPost, apiPut } from "shared/utils/general/api"
import { findDPI } from "shared/utils/general/dpi"
import { deepCopyObject } from "shared/utils/general/reducer"
import { FractalAPI, FractalHTTPCode } from "shared/types/api"
import { FractalTaskStatus } from "shared/types/containers"
import { FractalRoute } from "shared/types/navigation"
import { HOST_SERVICE_PORT } from "shared/constants/config"
import * as ContainerAction from "store/actions/container/sideEffects"
import { updateUser } from "store/actions/auth/pure"
import { updateTask, updateContainer } from "store/actions/container/pure"
import { DEFAULT } from "store/reducers/auth/default"
import { history } from "store/history"
import { DEFAULT as ContainerDefault } from "store/reducers/container/default"

function* createContainer() {
    /*
    Description:
        Sends POST request to create container with the correct region, DPI, etc.

    Arguments: none
    */
    const state = yield select()
    const userID = state.AuthReducer.user.userID
    const accessToken = state.AuthReducer.user.accessToken
    const region = state.ClientReducer.computerInfo.region

    // Get client DPI
    const dpi = findDPI()

    const body = {
        username: userID,
        region: region,
        app: "Google Chrome",
        dpi: dpi,
    }

    // Send container assign request
    const { json, success, response } = yield call(
        apiPost,
        FractalAPI.CONTAINER.ASSIGN,
        body,
        accessToken
    )

    // Get the Celery task ID
    if (success && json.ID) {
        yield put(updateTask({ taskID: json.ID }))
    } else {
        yield put(updateTask(ContainerDefault.task))
        yield put(
            updateTask({
                protocolKillSignal:
                    state.ContainerReducer.task.protocolKillSignal + 1,
                shouldLaunchProtocol: false,
            })
        )

        if (response && response.status === FractalHTTPCode.PAYMENT_REQUIRED) {
            history.push(FractalRoute.PAYMENT)
        } else {
            yield put(updateUser(deepCopyObject(DEFAULT.user)))
            history.push(FractalRoute.LOGIN)
        }
    }
}

function* getContainerInfo(action: { taskID: string; type: string }) {
    /*
    Description:
        Sends GET request to retrieve Celery task status (for container creation)

    Arguments: 
        action (Record): Redux action with Celery task ID
        type (string): identifier for this side effect
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

function* setHostServiceConfigToken(action: {
    ip: string
    port: number
    type: string
}) {
    /*
    Description:
        Sends PUT request to pass config encryption token to the host service

    Arguments:
        ip (string): IP address to host service
        port (number): port to pass to the host service // is this correct?
        type (string): identifier for this side effect
    */
    const state = yield select()
    const userID = state.AuthReducer.user.userID
    const accessToken = state.AuthReducer.user.access_token

    const body = {
        user_id: userID /* eslint-disable-line @typescript-eslint/camelcase */,
        host_port:
            action.port /* eslint-disable-line @typescript-eslint/camelcase */,
        config_encryption_token:
            state.AuthReducer.user
                .configToken /* eslint-disable-line @typescript-eslint/camelcase */,
        access_token: accessToken /* eslint-disable-line @typescript-eslint/camelcase */,
    }

    const { success } = yield call(
        apiPut,
        FractalAPI.HOST_SERVICE.SET_CONFIG_TOKEN,
        body,
        `https://${action.ip}:${HOST_SERVICE_PORT}`
    )

    if (!success) {
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
        takeEvery(
            ContainerAction.SET_HOST_SERVICE_CONFIG_TOKEN,
            setHostServiceConfigToken
        ),
    ])
}
