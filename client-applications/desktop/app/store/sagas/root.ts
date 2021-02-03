import { all } from "redux-saga/effects"

import AuthSaga from "store/sagas/auth"
import ContainerSaga from "store/sagas/container"

export default function* rootSaga() {
    yield all([AuthSaga(), ContainerSaga()])
}
