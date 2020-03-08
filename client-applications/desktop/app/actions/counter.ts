import { GetState, Dispatch } from '../reducers/types';

export const STORE_USER_INFO     = "STORE_USER_INFO";
export const TRACK_USER_ACTIVITY = "TRACK_USER_ACTIVITY";
export const LOGIN_USER          = "LOGIN_USER";
export const LOGIN_FAILED        = "LOGIN_FAILED";
export const CALCULATE_DISTANCE  = "CALCULATE_DISTANCE"
export const STORE_DISTANCE      = "STORE_DISTANCE";
export const SEND_FEEDBACK       = "SEND_FEEDBACK";
export const RESET_FEEDBACK      = "RESET_FEEDBACK";
export const SET_OS              = "SET_OS";
export const ASK_FEEDBACK        = "ASK_FEEDBACK";
export const CHANGE_WINDOW       = "CHANGE_WINDOW"

export function loginUser(username, password) {
	console.log("login action fired")
	return {
		type: LOGIN_USER,
		username,
		password
	}
}

export function storeUserInfo(username, public_ip, is_user) {
  return {
    type: STORE_USER_INFO,
    username,
    public_ip,
    is_user
  };
}

export function trackUserActivity(logon) {
	return {
		type: TRACK_USER_ACTIVITY,
		logon
	}
}

export function loginFailed() {
	return {
		type: LOGIN_FAILED
	}
}

export function calculateDistance(public_ip) {
	return {
		type: CALCULATE_DISTANCE,
		public_ip
	}
}

export function storeDistance(distance) {
	return {
		type: STORE_DISTANCE,
		distance
	}
}

export function sendFeedback(feedback) {
	return {
		type: SEND_FEEDBACK,
		feedback
	}
}

export function resetFeedback(reset) {
	return {
		type: RESET_FEEDBACK,
		reset
	}
}

export function setOS(os) {
  return {
    type: SET_OS,
    os
  }
}

export function askFeedback(ask) {
	return {
		type: ASK_FEEDBACK,
		ask
	}
}

export function changeWindow(window) {
	return {
		type: CHANGE_WINDOW,
		window
	}
}
