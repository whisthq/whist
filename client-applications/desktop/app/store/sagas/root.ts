import { all } from "redux-saga/effects"

import AuthSaga from "store/sagas/auth"
import ContainerSaga from "store/sagas/container"
import ClientSaga from "store/sagas/client"

export default function* rootSaga() {
    yield all([AuthSaga(), ContainerSaga(), ClientSaga()])
}
