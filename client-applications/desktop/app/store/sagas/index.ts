import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"
import { apiPost, apiGet } from "shared/utils/api"
import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"
import { history } from "store/configureStore"
import { generateMessage } from "shared/utils/loading"

import moment from "moment"

function* refreshAccess() {
    const state = yield select()
    const username = state.MainReducer.auth.username

    if (!username || username === "None" || username === "") {
        history.push("/")
        return
    }

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

function* validateAccessToken(action: any) {
    const { json, response } = yield call(
        apiGet,
        "/token/validate",
        action.accessToken
    )

    if (response && response.status === 200 && json && json.user) {
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

function* fetchPaymentInfo(action: any) {
    // const state = yield select()
    // const { json } = yield call(
    //     apiPost,
    //     `/stripe/retrieve`,
    //     {
    //         email: action.username,
    //     },
    //     state.MainReducer.auth.accessToken
    // )
    // if (json && json.accountLocked) {
    //     yield put(Action.updatePayment({ accountLocked: json.accountLocked }))
    // }
}

// an admin redux action that can be used
// if you set the admin redux state information correctly
// to test specific stuff that's not exposed to regular customers
function* createTestContainer(action: any) {
    // TODO what to do about this? is this necessary for not breaking stuff?
    yield put(
        Action.updateContainer({
            desiredAppID: action.app,
        })
    )

    const state = yield select()

    const username = state.MainReducer.auth.username

    const task_arn = state.MainReducer.admin.task_arn
    const region = state.MainReducer.admin.region
    const webserver = state.MainReducer.admin.webserver_url
    const cluster_name = state.MainReducer.admin.cluster

    console.log(
        `launching test container for user ${username} with task arn ${task_arn} in region ${region} with webserver ${webserver}`
    )

    if (!username || username === "None" || username === "") {
        history.push("/")
        return
    }

    var { json, response } = yield call(
        apiPost,
        `/aws_container/create_container`,
        {
            username: username,
            cluster_name: cluster_name,
            region_name: region,
            task_definition_arn: task_arn,
            //dpi not supported yet
        },
        state.MainReducer.auth.accessToken,
        webserver // webserver_url
    )

    if (response.status === 202) {
        const id = json.ID

        console.log(`container creation succeeded with id ${id}`)

        // TODO not sure if this is as desired?
        var { json, response } = yield call(
            apiGet,
            `/status/` + id,
            state.MainReducer.auth.accessToken,
            webserver
        )

        var progressSoFar = 0
        var secondsPassed = 0

        yield put(
            Action.updateLoading({
                percentLoaded: progressSoFar,
                statusMessage: "Preparing to stream test app",
            })
        )

        while (json && json.state !== "SUCCESS" && json.state !== "FAILURE") {
            console.log(
                `was able to get status for ${id} and json.state was ${json.state}`
            )

            if (secondsPassed % 1 === 0) {
                var { json, response } = yield call(
                    apiGet,
                    `/status/` + id,
                    state.MainReducer.auth.accessToken,
                    webserver
                )

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
            console.log(`success status!`)

            if (json.output) {
                console.log(`got output! it was ${JSON.stringify(json.output)}`)
                yield put(
                    Action.updateContainer({
                        container_id: json.output.container_id,
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
            var warning =
                `(${moment().format("hh:mm:ss")}) ` +
                `Unexpectedly lost connection with server. Cancelling...`
            progressSoFar = 0
            yield put(
                Action.updateLoading({
                    statusMessage: warning,
                    percentLoaded: progressSoFar,
                })
            )
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

function* createContainer(action: any) {
    yield put(
        Action.updateContainer({
            desiredAppID: action.app,
        })
    )

    const state = yield select()
    const username = state.MainReducer.auth.username

    var region = state.MainReducer.client.region
        ? state.MainReducer.client.region
        : "us-east-1"
    if (region === "us-east-2") {
        region = "us-east-1"
    }
    if (region === "us-west-2") {
        region = "us-west-1"
    }

    if (!username || username === "None" || username === "") {
        history.push("/")
        return
    }

    var { json, response } = yield call(
        apiPost,
        `/container/create`,
        {
            username: username,
            region: region,
            app: action.app,
            url: action.url,
            dpi: state.MainReducer.client.dpi,
        },
        state.MainReducer.auth.accessToken
    )

    if (response.status === 401 || response.status === 422) {
        yield call(refreshAccess)
        yield call(createContainer, action)
        return
    }

    if (response.status === 202) {
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

        while (json && json.state !== "SUCCESS" && json.state !== "FAILURE") {
            if (secondsPassed % 1 === 0) {
                var { json, response } = yield call(
                    apiGet,
                    `/status/` + id,
                    state.MainReducer.auth.accessToken
                )

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
                        container_id: json.output.container_id,
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
            var warning =
                `(${moment().format("hh:mm:ss")}) ` +
                `Unexpectedly lost connection with server. Cancelling...`
            progressSoFar = 0
            yield put(
                Action.updateLoading({
                    statusMessage: warning,
                    percentLoaded: progressSoFar,
                })
            )
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
        takeEvery(SideEffect.CREATE_CONTAINER, createContainer),
        takeEvery(SideEffect.CREATE_TEST_CONTAINER, createTestContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
        takeEvery(SideEffect.VALIDATE_ACCESS_TOKEN, validateAccessToken),
    ])
}
