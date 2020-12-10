import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"
import moment from "moment"

import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"

import { apiPost, apiGet, apiDelete } from "shared/utils/general/api"
import { history } from "store/history"
import { generateMessage } from "shared/components/loading"
import { FractalRoute } from "shared/types/navigation"
import { FractalAPI } from "shared/types/api"
import { FractalAuthCache } from "shared/types/cache"
import { config } from "shared/constants/config"
import { AWSRegion } from "shared/types/aws"

function* refreshAccess() {
    const state = yield select()
    const username = state.MainReducer.auth.username

    if (!username || username === "None" || username === "") {
        history.push(FractalRoute.LOGIN)
        return
    }

    const { json, success } = yield call(
        apiPost,
        FractalAPI.TOKEN.REFRESH,
        {},
        state.MainReducer.auth.refreshToken
    )
    if (success) {
        yield put(
            Action.updateAuth({
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
            })
        )
    }
}

function* fetchExternalApps() {
    const state = yield select()

    const { json, success } = yield call(
        apiGet,
        FractalAPI.APPS.EXTERNAL,
        state.MainReducer.auth.accessToken
    )

    if (success && json) {
        if (json.data) {
            yield put(
                Action.updateApps({
                    external: json.data,
                })
            )
        }
    } else {
        // console.log("failed to fetch external apps")
    }
}

function* fetchConnectedApps() {
    const state = yield select()

    const { json, success } = yield call(
        apiGet,
        FractalAPI.APPS.CONNECTED,
        state.MainReducer.auth.accessToken
    )

    if (success && json) {
        if (json.app_names) {
            yield put(
                Action.updateApps({
                    connected: json.app_names,
                })
            )
        }
    } else {
        // console.log("failed to fetch connected apps")
    }
}

function* validateAccessToken(action: { accessToken: string }) {
    const { json, success } = yield call(
        apiGet,
        FractalAPI.TOKEN.VALIDATE,
        action.accessToken
    )

    if (success && json.user) {
        const Store = require("electron-store")
        const storage = new Store()

        storage.set(FractalAuthCache.ACCESS_TOKEN, action.accessToken)

        yield put(
            Action.updateAuth({
                username: json.user.user_id,
                accessToken: action.accessToken,
                refreshToken: json.user.refresh_token,
                name: json.user.name,
            })
        )
        yield call(fetchExternalApps)
        yield call(fetchConnectedApps)
    } else {
        yield put(
            Action.updateAuth({
                loginWarning: true,
                loginMessage: "Login unsuccessful. Please try again.",
            })
        )
    }
}

function* createContainer(action: {
    app: string
    url: string
    test?: boolean
}) {
    const test = action.test
    const app = action.app
    const url = action.url

    yield put(
        Action.updateContainer({
            desiredAppID: app,
        })
    )

    const state = yield select()
    const username = state.MainReducer.auth.username

    const endpoint = test
        ? FractalAPI.CONTAINER.TEST_CREATE
        : FractalAPI.CONTAINER.ASSIGN
    const body = test
        ? {
              username: username,
              // eslint will yell otherwise... to avoid breaking server code we are disbabling
              /* eslint-disable */
              cluster_name: state.MainReducer.admin.cluster,
              region_name: state.MainReducer.admin.region,
              task_definition_arn: state.MainReducer.admin.taskArn,
              // dpi not supported yet
          }
        : {
              username: username,
              region: null,
              app: app,
              url: url,
              dpi: state.MainReducer.client.dpi,
          }
    const webserver = test
        ? state.MainReducer.admin.webserverUrl
        : config.url.WEBSERVER_URL

    if (!test) {
        let region = state.MainReducer.client.region
            ? state.MainReducer.client.region
            : AWSRegion.US_EAST_1
        if (region === AWSRegion.US_EAST_2) {
            region = AWSRegion.US_EAST_1
        }
        if (region === AWSRegion.US_WEST_2) {
            region = AWSRegion.US_WEST_1
        }
        body.region = region
    }

    if (!username || username === "None" || username === "") {
        history.push(FractalRoute.LOGIN)
        return
    }

    let { json, success } = yield call(
        apiPost,
        endpoint,
        body,
        state.MainReducer.auth.accessToken,
        webserver
    )

    if (!success) {
        yield call(refreshAccess)
        return
    }

    // TODO (adriano) add handlers for 404 (mainly for testing, low priority)

    const id = json.ID
    ;({ json, success } = yield call(
        apiGet,
        `/status/${id}`,
        state.MainReducer.auth.accessToken,
        webserver
    ))

    let progressSoFar = 0
    let secondsPassed = 0

    yield put(
        Action.updateLoading({
            percentLoaded: progressSoFar,
            statusMessage: `Preparing to stream ${action.app}`,
        })
    )

    while (json && json.state !== "SUCCESS" && json.state !== "FAILURE") {
        if (secondsPassed % 1 === 0) {
            ;({ json, success } = yield call(
                apiGet,
                `/status/${id}`,
                state.MainReducer.auth.accessToken,
                webserver
            ))

            if (!success) {
                const warning =
                    `(${moment().format("hh:mm:ss")}) ` +
                    "Unexpectedly lost connection with server. Please close the app and try again."

                progressSoFar = 0
                yield put(
                    Action.updateLoading({
                        percentLoaded: progressSoFar,
                        statusMessage: warning,
                    })
                )
                // TODO (adriano) we should not have to return in case it can try agin
                // however, there is no way to exit this loop right if they press the return button
                return
            }
        }

        // Update status message every six seconds
        if (success && secondsPassed > 0 && secondsPassed % 6 === 0) {
            yield put(
                Action.updateLoading({
                    statusMessage: generateMessage(),
                })
            )
        }

        // Update loading bar every second
        yield put(
            Action.updateLoading({
                percentLoaded: progressSoFar,
            })
        )
        progressSoFar = Math.min(99, progressSoFar + 1)

        yield delay(1000)
        secondsPassed += 1
    }
    // testing params : -w200 -h200 -p32262:32780,32263:32778,32273:32779 34.206.64.200
    if (json && json.state && json.state === "SUCCESS") {
        progressSoFar = 100

        yield put(
            Action.updateLoading({
                statusMessage: "Stream successfully started.",
                percentLoaded: progressSoFar,
            })
        )

        delay(1000) // so they can ready that above

        if (json.output) {
            yield put(
                Action.updateContainer({
                    containerID: json.output.container_id,
                    cluster: json.output.cluster,
                    port32262: json.output.port_32262,
                    port32263: json.output.port_32263,
                    port32273: json.output.port_32273,
                    location: json.output.location,
                    publicIP: json.output.ip,
                    secretKey: json.output.secret_key,
                    currentAppID: action.app,
                })
            )
        } else {
            yield put(
                Action.updateLoading({
                    statusMessage:
                        "Unexpectedly, server didn't send connection info. Please try again.",
                    percentLoaded: progressSoFar,
                })
            )
        }
    } else {
        const warning =
            `(${moment().format("hh:mm:ss")}) ` +
            `Unexpectedly lost connection with server. Trying again...`
        progressSoFar = 0
        yield put(
            Action.updateLoading({
                statusMessage: warning,
                percentLoaded: progressSoFar,
            })
        )
    }
}

function* submitFeedback(action: { feedback: string; feedbackType: string }) {
    const state = yield select()
    const { success } = yield call(
        apiPost,
        FractalAPI.MAIL.FEEDBACK,
        {
            username: state.MainReducer.auth.username,
            feedback: action.feedback,
            type: action.feedbackType,
        },
        state.MainReducer.auth.accessToken
    )

    if (!success) {
        yield call(refreshAccess)
        yield call(submitFeedback, action)
    }
}

function* disconnectApp(action: { app: string }) {
    const state = yield select()

    const { success } = yield call(
        apiDelete,
        FractalAPI.APPS.CONNECTED + "/" + action.app,
        state.MainReducer.auth.accessToken
    )

    if (success) {
        const connectedApps = state.MainReducer.apps.connected
        const index = connectedApps.indexOf(action.app)
        if (index > -1) {
            const newConnectedApps = Object.assign([], connectedApps)
            newConnectedApps.splice(index, 1)
            yield put(
                Action.updateApps({
                    connected: newConnectedApps,
                })
            )
        }
    } else {
        // console.log("failed to disconnect connected apps")
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(SideEffect.CREATE_CONTAINER, createContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
        takeEvery(SideEffect.VALIDATE_ACCESS_TOKEN, validateAccessToken),
        takeEvery(SideEffect.DISCONNECT_APP, disconnectApp),
    ])
}
