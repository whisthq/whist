import { put, take, takeLatest, takeEvery, all, call, select, delay } from 'redux-saga/effects';
import { channel } from 'redux-saga';
import { apiPost, apiGet } from '../utils/api.ts';
import * as Action from '../actions/counter';
import { configureStore, history } from '../store/configureStore';


function* loginUser(action) {
  console.log("saga called")
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/user/login', {
    username: action.username,
    password: action.password
  })

  console.log(json)

  if(json.username) {
    yield put(Action.storeUserInfo(action.username, json.public_ip));
    history.push("/counter");
  } else {
    yield put(Action.loginFailed());
  }
}

function* trackUserActivity(action) {
  const state = yield select()
  if(action.logon) {
    const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/tracker/logon', {
      username: state.counter.username
    })
  } else {
    const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/tracker/logoff', {
      username: state.counter.username
    })
  }
}


export default function* rootSaga() {
 	yield all([
     takeEvery(Action.TRACK_USER_ACTIVITY, trackUserActivity),
     takeEvery(Action.LOGIN_USER, loginUser)
	]);
}
