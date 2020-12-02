import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"
import moment from "moment"

import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"

import { apiPost, apiGet } from "shared/utils/api"
import { history } from "store/history"
import { generateMessage } from "shared/utils/loading"
import { FractalRoute } from "shared/enums/navigation"
import { FractalAPI } from "shared/enums/api"
import { AWSRegion } from "shared/enums/aws"

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

function* validateAccessToken<T extends {}>(action: { body: T }) {
    const { json, success } = yield call(
        apiGet,
        FractalAPI.TOKEN.VALIDATE,
        action.accessToken
    )

    if (success && json.user) {
        const Store = require("electron-store")
        const storage = new Store()

        storage.set("accessToken", action.accessToken)

        yield put(
            Action.updateAuth({
                username: json.user.user_id,
                accessToken: action.accessToken,
                refreshToken: json.user.refresh_token,
            })
        )
    } else {
        yield put(
            Action.updateAuth({
                loginWarning: true,
                loginMessage: "Login unsuccessful. Please try again.",
            })
        )
    }
}

function* createContainer<T extends {}>(action: { body: T }) {
    yield put(
        Action.updateContainer({
            desiredAppID: action.app,
        })
    )

    const state = yield select()
    const username = state.MainReducer.auth.username

    let region = state.MainReducer.client.region
        ? state.MainReducer.client.region
        : AWSRegion.US_EAST_1
    if (region === AWSRegion.US_EAST_2) {
        region = AWSRegion.US_EAST_1
    }
    if (region === AWSRegion.US_WEST_2) {
        region = AWSRegion.US_WEST_1
    }

    if (!username || username === "None" || username === "") {
        history.push(FractalRoute.LOGIN)
        return
    }

    const data = yield call(
        apiPost,
        FractalAPI.CONTAINER.CREATE,
        {
            username: username,
            region: region,
            app: action.app,
            url: action.url,
            dpi: state.MainReducer.client.dpi,
        },
        state.MainReducer.auth.accessToken
    )

    let { json } = data
    const { success } = data

    if (!success) {
        yield call(refreshAccess)
        yield call(createContainer, action)
        return
    }

    const id = json.ID
    ;({ json, response } = yield call(
        apiGet,
        `/status/${id}`,
        state.MainReducer.auth.accessToken
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
            ;({ response } = yield call(
                apiGet,
                `/status/${id}`,
                state.MainReducer.auth.accessToken
            ))

            if (response && response.status && response.status === 500) {
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
            }
        }

        // Update status message every six seconds
        if (secondsPassed > 0 && secondsPassed % 6 === 0) {
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
        if (json.output) {
            yield put(
                Action.updateContainer({
                    containerID: json.output.containerID,
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
        }

        progressSoFar = 100

        yield put(
            Action.updateLoading({
                statusMessage: "Stream successfully started.",
                percentLoaded: progressSoFar,
            })
        )
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

function* submitFeedback<T extends {}>(action: { body: T }) {
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

export default function* rootSaga() {
    yield all([
        takeEvery(SideEffect.CREATE_CONTAINER, createContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
        takeEvery(SideEffect.VALIDATE_ACCESS_TOKEN, validateAccessToken),
    ])
}
