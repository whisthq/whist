import { put, takeEvery, all, call, select } from "redux-saga/effects"
import moment from "moment"

import * as Action from "store/actions/pure"
import * as SideEffect from "store/actions/sideEffects"

import { apiPost, apiGet } from "shared/utils/api"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { FractalAPI } from "shared/types/api"
import { FractalAuthCache } from "shared/types/cache"
import { config } from "shared/constants/config"
import { AWSRegion } from "shared/types/aws"
import { FractalStatus } from "shared/types/containers"

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

    // console.log(`create container saga, test, app, url : ${test}, ${app}, ${url}`)

    yield put(
        Action.updateContainer({
            desiredAppID: app,
        })
    )

    // so we know that we are launching the test/admin version to shut down with the right webserver
    if (test) {
        yield put(
            Action.updateAdmin({
                launched: true,
            })
        )
    }

    const state = yield select()
    const username = state.MainReducer.auth.username

    const endpoint = test
        ? FractalAPI.CONTAINER.TEST_CREATE
        : FractalAPI.CONTAINER.CREATE
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
        yield call(createContainer, action)
        return
    } else {
        const id = json.ID
        yield put(Action.updateContainer({
            currentAppID: action.app,
            statusID: id,
        }))
    }

    // there used to be a while loop here that did things, now
    // we are going to use the websocket hasura/graphql endpoints
}

// this should potentially be replaced with a graphQL query if we can get the right permissions
// and unique keys
function *getStatus(action: {id: string}) {
    const id = action.id
    const state = yield select()

    const { json, response } = yield call(
        apiGet,
        `/status/` + id,
        state.MainReducer.auth.accessToken
    )

    if (json && json.state === FractalStatus.SUCCESS) {
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
                })
            )

        const progressSoFar = 100

        yield put(
            Action.updateLoading({
                statusMessage: "Stream successfully started.",
                percentLoaded: progressSoFar,
            })
        )
    } else {
        const warning = json && json.state === FractalStatus.FAILURE ? "Unexpectedly failed to start stream. Close and try again." 
            : response && response.status && response.status === 500 
                ? `(${moment().format("hh:mm:ss")}) ` + 
                  "Unexpectedly lost connection with server. Please close the app and try again."
                : "Server unexpectedly not responding. Close the app and try again."

        const progressSoFar = 0
        yield put(
            Action.updateLoading({
                percentLoaded: progressSoFar,
                statusMessage: warning,
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

function* deleteContainer(action: {containerID: string; privateKey: string; test: boolean}) {
    const state= yield select()

    const test = action.test
    const containerID = action.containerID
    const private_key = action.privateKey

    if (test) {
        yield put(Action.updateAdmin({
            launched: false,
        }))
    }
    
    const username = state.MainReducer.auth.username
    const webserver = test
        ? state.MainReducer.admin.webserverUrl
        : config.url.WEBSERVER_URL

    const { success } = yield call(
        apiPost,
        FractalAPI.CONTAINER.DELETE,
        {
            // note how the webserver expects this naming format
            /* eslint-disable */
            container_id: containerID,
            hex_aes_private_key: private_key,
            username: username,
        },
        state.MainReducer.auth.accessToken,
        webserver,
    )

    if (!success) {
        yield call(refreshAccess)
        yield call(deleteContainer, action)
    } else {
        yield put(Action.updateContainer({
            currentAppID: null,
            statusID: null,
        }))
    }
}

export default function* rootSaga() {
    yield all([
        takeEvery(SideEffect.CREATE_CONTAINER, createContainer),
        takeEvery(SideEffect.SUBMIT_FEEDBACK, submitFeedback),
        takeEvery(SideEffect.VALIDATE_ACCESS_TOKEN, validateAccessToken),
        takeEvery(SideEffect.DELETE_CONTAINER, deleteContainer),
        takeEvery(SideEffect.GET_STATUS, getStatus),
    ])
}
