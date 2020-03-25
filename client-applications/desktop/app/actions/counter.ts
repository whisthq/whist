import { GetState, Dispatch } from '../reducers/types'

export const STORE_USERNAME      = "STORE_USERNAME"
export const STORE_IP            = "STORE_IP"
export const STORE_IS_USER       = "STORE_IS_USER"
export const TRACK_USER_ACTIVITY = "TRACK_USER_ACTIVITY"
export const LOGIN_USER          = "LOGIN_USER"
export const LOGIN_FAILED        = "LOGIN_FAILED"
export const CALCULATE_DISTANCE  = "CALCULATE_DISTANCE"
export const STORE_DISTANCE      = "STORE_DISTANCE"
export const SEND_FEEDBACK       = "SEND_FEEDBACK"
export const RESET_FEEDBACK      = "RESET_FEEDBACK"
export const SET_OS              = "SET_OS"
export const ASK_FEEDBACK        = "ASK_FEEDBACK"
export const CHANGE_WINDOW       = "CHANGE_WINDOW"
export const LOGIN_STUDIO        = "LOGIN_STUDIO"
export const PING_IPINFO	     = "PING_IPINFO"
export const STORE_IPINFO        = "STORE_IPINFO"
export const FETCH_COMPUTERS     = "FETCH_COMPUTERS"
export const STORE_COMPUTERS     = "STORE_COMPUTERS"
export const FETCH_VMS           = "FETCH_VMS"
export const FETCH_VM_STATUS     = "FETCH_VM_STATUS"


export function loginUser(username, password) {
	console.log("login action fired")
	return {
		type: LOGIN_USER,
		username,
		password
	}
}

export function storeUsername(username) {
  return {
    type: STORE_USERNAME,
    username
  };
}

export function storeIP(ip) {
  return {
    type: STORE_IP,
    ip
  };
}

export function storeIsUser(isUser) {
  return {
    type: STORE_IS_USER,
    isUser
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

export function loginStudio(username, password) {
	return {
		type: LOGIN_STUDIO,
		username, 
		password
	}
}

export function pingIPInfo(id) {
	return {
		type: PING_IPINFO,
		id
	}
}

export function storeIPInfo(payload, id) {
	return {
		type: STORE_IPINFO,
		payload,
		id
	}
}

export function fetchComputers() {
	return {
		type: FETCH_COMPUTERS
	}
}

export function storeComputers(payload) {
	return {
		type: STORE_COMPUTERS,
		payload
	}
}

export function fetchVMs(username) {
	return {
		type: FETCH_VMS,
		username
	}
}

export function fetchVMStatus(status) {
	return {
		type: FETCH_VM_STATUS,
		status
	}
}