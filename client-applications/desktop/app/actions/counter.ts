import { GetState, Dispatch } from '../reducers/types'

export const LOGOUT              = "LOGOUT"
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
export const FETCH_DISK          = "FETCH_DISK"
export const FETCH_DISK_STATUS   = "FETCH_DISK_STATUS"
export const STORE_DISK_NAME     = "STORE_DISK_NAME"
export const ATTACH_DISK         = "ATTACH_DISK"
export const FETCH_VM            = "FETCH_VM"
export const STORE_JWT           = "STORE_JWT"
export const CREATE_DISK         = "CREATE_DISK"
export const STORE_PAYMENT_INFO  = "STORE_PAYMENT_INFO"
export const STORE_PROMO_CODE    = "STORE_PROMO_CODE"
export const RESTART_PC          = "RESTART_PC"
export const VM_RESTARTED        = "VM_RESTARTED"
export const SEND_LOGS           = "SEND_LOGS"
export const CHANGE_STATUS_MESSAGE = "CHANGE_STATUS_MESSAGE"
export const UPDATE_FOUND        = "UPDATE_FOUND"
export const ATTACH_DISK        = "ATTACH_DISK"

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

export function loginFailed(warning) {
	return {
		type: LOGIN_FAILED,
		warning
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

export function fetchDisk(username) {
	return {
		type: FETCH_DISK,
		username
	}
}

export function fetchDiskStatus(status) {
	return {
		type: FETCH_DISK_STATUS,
		status
	}
}

export function storeDiskName(disk, location) {
	return {
		type: STORE_DISK_NAME,
		disk, location
	}
}

export function attachDisk() {
	return {
		type: ATTACH_DISK
	}
}

export function fetchVM(id) {
	return {
		type: FETCH_VM,
		id
	}
}

export function storeJWT(access_token, refresh_token) {
	return {
		type: STORE_JWT,
		access_token,
		refresh_token
	}
}

export function createDisk() {
	return {
		type: CREATE_DISK
	}
}

export function storePaymentInfo(account_locked) {
	return {
		type: STORE_PAYMENT_INFO,
		account_locked
	}
}

export function storePromoCode(code) {
	return {
		type: STORE_PROMO_CODE,
		code
	}
}

export function logout() {
	return {
		type: LOGOUT
	}
}

export function restartPC() {
	return {
		type: RESTART_PC
	}
}

export function vmRestarted(status) {
	console.log("VM STATUS IS NOW")
	console.log(status)
	return {
		type: VM_RESTARTED
	}
}

export function sendLogs(connection_id, logs) {
	return {
		type: SEND_LOGS,
		connection_id,
		logs
	}
} 

export function changeStatusMessage(status_message) {
	return {
		type: CHANGE_STATUS_MESSAGE,
		status_message
	}
}

export function updateFound(update) {
	return {
		type: UPDATE_FOUND,
		update
	}
}

export function attachDisk(update) {
	return {
		type: ATTACH_DISK,
		update
	}
}