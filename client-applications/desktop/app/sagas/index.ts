import { put, take, takeLatest, takeEvery, all, call, select, delay } from 'redux-saga/effects';
import { channel } from 'redux-saga';
import { apiPost, apiGet } from '../utils/api.ts';
import * as Action from '../actions/counter';
import { configureStore, history } from '../store/configureStore';


function* loginUser(action) {
  const {json, response} = yield call(apiPost, 'https://cube-celery-vm.herokuapp.com/user/login', {
    username: action.username,
    password: action.password
  })

  if(json.username) {
    if(!json.is_user) {
      action.username = action.username.substring(0, action.username.length - 4) + " (Admin)"
    }
    yield put(Action.storeUserInfo(action.username, json.public_ip, json.is_user));
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

function* sendFeedback(action) {
  const state = yield select()
  const {json, response} = yield call(apiPost, 'https://fractal-mail-server.herokuapp.com/feedback', {
    username: state.counter.username,
    feedback: action.feedback
  })
  yield put(Action.resetFeedback(true))
}


export default function* rootSaga() {
 	yield all([
     takeEvery(Action.TRACK_USER_ACTIVITY, trackUserActivity),
     takeEvery(Action.LOGIN_USER, loginUser),
     takeEvery(Action.SEND_FEEDBACK, sendFeedback)
	]);
}
