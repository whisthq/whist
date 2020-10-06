import { put, takeEvery, all, call, select, delay } from 'redux-saga/effects'
import { apiPost, apiGet } from 'utils/api'
import * as Action from 'store/actions/pure'
import * as SideEffect from 'store/actions/sideEffects'
import { history } from 'store/configureStore'

import { config } from 'constants/config'

import moment from 'moment'

function* loginUser(action: any) {
    if (action.username !== '' && action.password !== '') {
        const { json } = yield call(
            apiPost,
            `${config.url.PRIMARY_SERVER}/account/login`,
            {
                username: action.username,
                password: action.password,
            }
        )

        console.log(json)

        if (json && json.verified) {
            yield put(
                Action.updateAuth({
                    accessToken: json.access_token,
                    refreshToken: json.refresh_token,
                    username: action.username,
                })
            )
            yield call(fetchPaymentInfo, action)
            yield call(getPromoCode, action)
            history.push('/loading')
        } else {
            yield put(Action.updateAuth({ loginWarning: true }))
        }
    } else {
        yield put(Action.updateAuth({ loginWarning: true }))
    }
}

function* googleLogin(action) {
    yield select()

    if (action.code) {
        const { json } = yield call(
            apiPost,
            `${config.url.PRIMARY_SERVER}/google/login`,
            {
                code: action.code,
                clientApp: true,
            }
        )
        if (json) {
            if (json.status === 200) {
                yield put(
                    Action.updateAuth({
                        accessToken: json.accessToken,
                        refreshToken: json.refreshToken,
                        username: json.username,
                    })
                )
                yield call(fetchPaymentInfo, { username: json.username })
                yield call(getPromoCode, { username: json.username })
                history.push('/loading')
            } else {
                yield put(Action.updateAuth(true))
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
        `${config.url.PRIMARY_SERVER}/stripe/retrieve`,
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
        `${config.url.PRIMARY_SERVER}/account/code?username=${action.username}`,
        state.MainReducer.auth.accessToken
    )

    console.log(json)

    if (json && json.status === 200) {
        yield put(Action.updatePayment({ promoCode: json.code }))
    }
}

function* fetchContainer(action: any) {
    const state = yield select()
    const username = state.MainReducer.auth.username
    const app = 'test'
    console.log(username)
    var { json, response } = yield call(
        apiPost,
        `${config.url.PRIMARY_SERVER}/container/create`,
        { username: username, app: app },
        state.MainReducer.auth.accessToken
    )
    // var { json, response } = yield call(
    //     apiGet,
    //     `${config.url.PRIMARY_SERVER}/dummy`,
    //     state.MainReducer.auth.accessToken
    // )

    const id = json.ID
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/status/` + id,
        state.MainReducer.auth.accessToken
    )
    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.MainReducer.auth.accessToken
        )
        console.log(json)

        if (response && response.status && response.status === 500) {
            const warning =
                `(${moment().format('hh:mm:ss')}) ` +
                'Unexpectedly lost connection with server. Please close the app and try again.'

            yield put(
                Action.updateLoading({
                    percentLoaded: 0,
                    statusMessage: warning,
                })
            )
        }

        if (json && json.state === 'PENDING' && json.output) {
            // NOTE: actual container/create endpoint does not currently return progress
            var message = json.output.msg
            // var percent = json.output.progress
            if (message) {
                yield put(
                    Action.updateLoading({
                        percentLoaded: 50,
                        statusMessage: message,
                    })
                )
            }
        }

        yield delay(5000)
    }
    // testing params : -w200 -h200 -p32262:32780,32263:32778,32273:32779 34.206.64.200
    if (json && json.state && json.state === 'SUCCESS') {
        if (json.output) {
            // TODO (adriano) these should be removed once we are ready to plug and play
            const test_container_id = 'container_id' // TODO
            const test_cluster = 'cluster' // TODO
            const test_ip = '34.206.64.200'
            const test_port32262 = '32780'
            const test_port32263 = '32778'
            const test_port32273 = '32779'
            const test_location = 'location' // TODO

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
                ? json.output.port32262
                : test_port32262
            const port32263 = json.output.port_32263
                ? json.output.port32263
                : test_port32263
            const port32273 = json.output.port_32273
                ? json.output.port32273
                : test_port32273
            const location = json.output.location
                ? json.output.location
                : test_location

            yield put(
                Action.updateContainer({
                    container_id: container_id,
                    cluster: cluster,
                    port32262: port_32262,
                    port32263: port_32263,
                    port32273: port_32273,
                    location: location,
                    publicIP: ip,
                })
            )
            yield call(deleteContainer, {
                username: username,
                container_id: container_id,
            })
        }

        yield put(
            Action.updateLoading({
                statusMessage: 'Successfully loaded resources.',
                percentLoaded: 100,
            })
        )
    } else {
        var warning =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        yield put(
            Action.updateLoading({
                statusMessage: warning,
            })
        )
    }
}

function* deleteContainer(action: any) {
    const state = yield select()
    var { json, response } = yield call(
        apiPost,
        `${config.url.PRIMARY_SERVER}/container/delete`,
        { username: action.username, container_id: action.container_id },
        state.MainReducer.auth.accessToken
    )
    yield put(
        Action.updateLoading({
            statusMessage: 'Starting to shut down resources.',
            percentLoaded: 0,
        })
    )

    const id = json.ID
    var { json, response } = yield call(
        apiGet,
        `${config.url.PRIMARY_SERVER}/status/` + id,
        state.MainReducer.auth.accessToken
    )

    while (json.state !== 'SUCCESS' && json.state !== 'FAILURE') {
        var { json, response } = yield call(
            apiGet,
            `${config.url.PRIMARY_SERVER}/status/` + id,
            state.MainReducer.auth.accessToken
        )

        if (response && response.status && response.status === 500) {
            const warning =
                `(${moment().format('hh:mm:ss')}) ` +
                'Unexpectedly lost connection with server. Please close the app and try again.'
            yield put(
                Action.updateLoading({
                    statusMessage: warning,
                    percentLoaded: 0,
                })
            )
        }

        if (json && json.state === 'PENDING' && json.output) {
            var message = json.output.msg
            if (message) {
                yield put(
                    Action.updateLoading({
                        statusMessage: message,
                        percentLoaded: 50,
                    })
                )
            }
        }

        yield delay(5000)
    }

    if (json && json.state && json.state === 'SUCCESS') {
        yield put(
            Action.updateLoading({
                statusMessage: 'Successfully delete resources.',
                percentLoaded: 100,
            })
        )
    } else {
        var warning =
            `(${moment().format('hh:mm:ss')}) ` +
            `Unexpectedly lost connection with server. Please close the app and try again.`
        if (json.output && json.output.msg) {
            warning = json.output.msg
        }
        yield put(
            Action.updateLoading({
                statusMessage: warning,
            })
        )
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(SideEffect.LOGIN_USER, loginUser),
        takeEvery(SideEffect.GOOGLE_LOGIN, googleLogin),
        takeEvery(SideEffect.FETCH_CONTAINER, fetchContainer),
        takeEvery(SideEffect.DELETE_CONTAINER, deleteContainer),
    ])
}
