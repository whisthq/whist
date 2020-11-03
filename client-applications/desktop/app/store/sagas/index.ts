import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"
import { apiPost, apiGet } from "shared/utils/api"
import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"
import { history } from "store/configureStore"
import { generateMessage } from "shared/utils/loading"

import moment from "moment"

function* refreshAccess() {
    const state = yield select()
    const { json } = yield call(
        apiPost,
        `/token/refresh`,
        {},
        state.MainReducer.auth.refreshToken
    )
    if (json) {
        yield put(
            Action.updateAuth({
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
            })
        )
    }
}

function* loginUser(action: any) {
    if (action.username !== "" && action.password !== "") {
        const { json } = yield call(apiPost, `/account/login`, {
            username: action.username,
            password: action.password,
        })

        if (json && json.verified) {
            yield put(
                Action.updateAuth({
                    accessToken: json.access_token,
                    refreshToken: json.refresh_token,
                    username: action.username,
                    name: json.name,
                })
            )
            if (action.rememberMe) {
                const storage = require("electron-json-storage")
                storage.set("credentials", {
                    username: action.username,
                    accessToken: json.access_token,
                    refreshToken: json.refresh_token,
                })
            }
            yield call(fetchPaymentInfo, action)
            yield call(getPromoCode, action)
            history.push("/dashboard")
        } else {
            yield put(Action.updateAuth({ loginWarning: true }))
        }
    } else {
        yield put(Action.updateAuth({ loginWarning: true }))
    }
}

function* googleLogin(action: any) {
    yield select()

    if (action.code) {
        const { json, response } = yield call(apiPost, `/google/login`, {
            code: action.code,
            clientApp: true,
        })
        if (json) {
            if (response.status === 200) {
                yield put(
                    Action.updateAuth({
                        accessToken: json.access_token,
                        refreshToken: json.refresh_token,
                        username: json.username,
                        name: json.name,
                    })
                )

                if (action.rememberMe) {
                    const storage = require("electron-json-storage")
                    storage.set("credentials", {
                        username: json.username,
                        accessToken: json.access_token,
                        refreshToken: json.refresh_token,
                    })
                }
                yield call(fetchPaymentInfo, { username: json.username })
                yield call(getPromoCode, { username: json.username })
                history.push("/dashboard")
            } else {
                yield put(Action.updateAuth({ loginWarning: true }))
            }
        } else {
            yield put(Action.updateAuth({ loginWarning: true }))
        }
    } else {
        yield put(Action.updateAuth({ loginWarning: true }))
    }
}

function* rememberMeLogin(action: any) {
    const { json } = yield call(apiPost, `/account/auto_login`, {
        username: action.username,
    })

    if (json) {
        if (json.status === 200) {
            yield put(
                Action.updateAuth({
                    accessToken: json.access_token,
                    refreshToken: json.refresh_token,
                    username: action.username,
                    name: json.name,
                })
            )
            yield call(fetchPaymentInfo, action)
            yield call(getPromoCode, action)
            history.push("/dashboard")
        } else {
            yield put(Action.updateAuth({ loginWarning: true }))
        }
    } else {
        yield put(Action.updateAuth({ loginWarning: true }))
    }
}

function* fetchPaymentInfo(action: any) {
    const state = yield select()
    const { json } = yield call(
        apiPost,
        `/stripe/retrieve`,
        {
            username: action.username,
        },
        state.MainReducer.auth.accessToken
    )

    if (json && json.accountLocked) {
        yield put(Action.updatePayment({ accountLocked: json.accountLocked }))
    }
}

function* getPromoCode(action: any) {
    const state = yield select()
    const { json } = yield call(
        apiGet,
        `/account/code?username=${action.username}`,
        state.MainReducer.auth.accessToken
    )

    if (json && json.status === 200) {
        yield put(Action.updatePayment({ promoCode: json.code }))
    }
}

function* fetchContainer(action: any, retries?: number) {
    if (!retries || retries < 2) {
        history.push("/loading")
        const state = yield select()
        const username = state.MainReducer.auth.username
        // if they are super far we'll just default them to us east and hope for the best
        const region = state.MainReducer.client.region
            ? state.MainReducer.client.region
            : "us-east-1"
        const app = action.app

        var { json, response } = yield call(
            apiPost,
            `/container/create`,
            { username: username, region: region, app: app },
            state.MainReducer.auth.accessToken
        )

        if (response.status === 401 || response.status === 422) {
            yield call(refreshAccess)
            yield call(fetchContainer, action)
            return
        }

        const id = json.ID
        var { json, response } = yield call(
            apiGet,
            `/status/` + id,
            state.MainReducer.auth.accessToken
        )

        var progressSoFar = 0
        var secondsPassed = 0

        yield put(
            Action.updateLoading({
                percentLoaded: progressSoFar,
                statusMessage: "Preparing to stream " + action.app,
            })
        )

        while (json.state !== "SUCCESS" && json.state !== "FAILURE") {
            if (secondsPassed % 3 === 0) {
                var { json, response } = yield call(
                    apiGet,
                    `/status/` + id,
                    state.MainReducer.auth.accessToken
                )

                if (response && response.status && response.status === 500) {
                    const warning =
                        `(${moment().format("hh:mm:ss")}) ` +
                        "Unexpectedly lost connection with server. Please close the app and try again."

                    yield put(
                        Action.updateLoading({
                            percentLoaded: 0,
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
                        container_id: json.output.container_id,
                        cluster: json.output.cluster,
                        port32262: json.output.port_32262,
                        port32263: json.output.port_32263,
                        port32273: json.output.port_32273,
                        location: json.output.location,
                        publicIP: json.output.ip,
                    })
                )
            }

            yield put(
                Action.updateLoading({
                    statusMessage: "Stream successfully started.",
                    percentLoaded: 100,
                })
            )
        } else {
            var warning =
                `(${moment().format("hh:mm:ss")}) ` +
                `Unexpectedly lost connection with server. Trying again...`
            yield put(
                Action.updateLoading({
                    statusMessage: warning,
                })
            )
            yield delay(15000)
            yield call(fetchContainer, action, retries ? retries + 1 : 1)
        }
    } else {
        yield put(
            Action.updateLoading({
                statusMessage:
                    "Server unexpectedly not responding. Close the app and try again.",
            })
        )
    }
}

function* submitFeedback(action: any) {
    const state = yield select()
    const { response } = yield call(
        apiPost,
        `/mail/feedback`,
        {
            username: state.MainReducer.auth.username,
            feedback: action.feedback,
            type: action.feedback_type,
        },
        state.MainReducer.auth.accessToken
    )

    if (response.status === 401 || response.status === 422) {
        yield call(refreshAccess)
        yield call(submitFeedback, action)
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(SideEffect.LOGIN_USER, loginUser),
        takeEvery(SideEffect.GOOGLE_LOGIN, googleLogin),
        takeEvery(SideEffect.REMEMBER_ME_LOGIN, rememberMeLogin),
        takeEvery(SideEffect.FETCH_CONTAINER, fetchContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
    ])
}
