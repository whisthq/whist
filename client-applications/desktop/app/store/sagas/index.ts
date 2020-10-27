import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"
import { apiPost, apiGet } from "shared/utils/api"
import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"
import { history } from "store/configureStore"

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

        const storage = require("electron-json-storage")
        storage.set("credentials", {
            username: state.MainReducer.auth.username,
            accessToken: json.access_token,
            refreshToken: json.refresh_token,
        })
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

function* fetchContainer(action: any) {
    history.push("/loading")
    const state = yield select()
    // const username = state.MainReducer.auth.username
    const username = "ming@fractalcomputers.com"
    //const region = state.MainReducer.client.region
    const region = "us-east-1"
    const app = action.app

    var { json, response } = yield call(
        apiPost,
        `/container/create`,
        { username: username, region: region, app: app },
        state.MainReducer.auth.accessToken
    )
    // var { json, response } = yield call(
    //     apiGet,
    //     `/dummy`,
    //     state.MainReducer.auth.accessToken
    // )

    const id = json.ID
    var { json, response } = yield call(
        apiGet,
        `/status/` + id,
        state.MainReducer.auth.accessToken
    )
    while (json.state !== "SUCCESS" && json.state !== "FAILURE") {
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

        if (json && json.state === "PENDING" && json.output) {
            // NOTE: actual container/create endpoint does not currently return progress
            // var message = json.output.msg
            // var percent = json.output.progress
            // if (message) {
            //     yield put(
            //         Action.updateLoading({
            //             percentLoaded: 50,
            //             statusMessage: message,
            //         })
            //     )
            // }
            yield put(
                Action.updateLoading({
                    percentLoaded: 50,
                    statusMessage: "Preparing your app",
                })
            )
        }

        yield delay(5000)
    }
    // testing params : -w200 -h200 -p32262:32780,32263:32778,32273:32779 34.206.64.200
    if (json && json.state && json.state === "SUCCESS") {
        if (json.output) {
            // TODO (adriano) these should be removed once we are ready to plug and play
            const test_container_id = "container_id" // TODO
            const test_cluster = "cluster" // TODO
            const test_ip = "34.206.64.200"
            const test_port32262 = "32780"
            const test_port32263 = "32778"
            const test_port32273 = "32779"
            const test_location = "location" // TODO

            const test_width = 200
            const test_height = 200
            // const test_codec = 'h264'

            // TODO (adriano) add a signaling param or something to say that it's the 'test'
            // or it's the actual thing
            const container_id = json.output.container_id
                ? json.output.container_id
                : test_container_id
            const cluster = json.output.cluster
                ? json.output.cluster
                : test_cluster
            const ip = json.output.ip ? json.output.ip : test_ip
            const port32262 = json.output.port_32262
                ? json.output.port_32262
                : test_port32262
            const port32263 = json.output.port_32263
                ? json.output.port_32263
                : test_port32263
            const port32273 = json.output.port_32273
                ? json.output.port_32273
                : test_port32273
            const location = json.output.location
                ? json.output.location
                : test_location

            yield put(
                Action.updateContainer({
                    container_id: container_id,
                    cluster: cluster,
                    port32262: port32262,
                    port32263: port32263,
                    port32273: port32273,
                    location: location,
                    publicIP: ip,
                })
            )
        }

        yield put(
            Action.updateLoading({
                statusMessage: "Successfully loaded resources.",
                percentLoaded: 100,
            })
        )
    } else {
        var warning =
            `(${moment().format("hh:mm:ss")}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        yield put(
            Action.updateLoading({
                statusMessage: warning,
            })
        )
    }
}

function* deleteContainer(action: any) {
    console.log("sending delete container")
    const state = yield select()
    yield call(
        apiPost,
        `/container/delete`,
        { username: action.username, container_id: action.container_id },
        state.MainReducer.auth.accessToken
    )
    history.push("/dashboard")
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
        takeEvery(SideEffect.FETCH_CONTAINER, fetchContainer),
        takeEvery(SideEffect.DELETE_CONTAINER, deleteContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
    ])
}
