import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FaEdit } from "react-icons/fa"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import Input from "shared/components/input"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import * as PureAuthAction from "store/actions/auth/pure"
import { updatePassword } from "store/actions/auth/sideEffects"
import { checkPasswordVerbose } from "pages/auth/constants/authHelpers"

import "styles/profile.css"

const PasswordForm = (props: any) => {
    const { dispatch, user, authFlow } = props

    const [changingPassword, setChangingPassword] = useState(false)
    const [password, setPassword] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [newPassword, setNewPassword] = useState("")
    const [newPasswordWarning, setNewPasswordWarning] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [savingPassword, setSavingPassword] = useState(false)
    const [savedPassword, setSavedPassword] = useState(false)

    // Reset password warning when password is updated
    const changePassword = (evt: any): any => {
        evt.persist()
        setPassword(evt.target.value)
        if (passwordWarning !== "") {
            setPasswordWarning("")
        }
    }

    const changeNewPassword = (evt: any): any => {
        evt.persist()
        setNewPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: any): any => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    // Reset redux state if already contains values, then call endpoint to update password
    const savePassword = () => {
        setSavingPassword(true)
        if (authFlow.resetDone || authFlow.passwordVerified) {
            dispatch(
                PureAuthAction.updateAuthFlow({
                    resetDone: false,
                    passwordVerified: null,
                })
            )
        }
        dispatch(updatePassword(password, newPassword))
    }

    // Update warning for new password
    useEffect(() => {
        setNewPasswordWarning(checkPasswordVerbose(newPassword))
    }, [newPassword])

    // Update warning for confirm password
    useEffect(() => {
        if (
            confirmPassword !== newPassword &&
            newPassword.length > 0 &&
            confirmPassword.length > 0
        ) {
            setConfirmPasswordWarning("Doesn't match")
        } else {
            setConfirmPasswordWarning("")
        }
    }, [confirmPassword, newPassword])

    // Close editing mode for password if successfully changed, or show warning
    useEffect(() => {
        if (authFlow.passwordVerified === "success" && authFlow.resetDone) {
            // Check necessary so savedPassword isn't set to true on initial page load
            if (changingPassword) {
                setSavedPassword(true)
            }
            setSavingPassword(false)
            setChangingPassword(false)
            setPassword("")
            setNewPassword("")
            setConfirmPassword("")
        } else if (authFlow.passwordVerified === "failed") {
            setPasswordWarning("Incorrect current password")
            setSavingPassword(false)
        }
    }, [authFlow.passwordVerified, authFlow.resetDone, changingPassword])

    return (
        <>
            <div className="section-title">Password</div>
            <div className="section-info">
                {user.usingGoogleLogin && (
                    <div style={{ fontStyle: "italic" }}>
                        Logged in through Google
                    </div>
                )}
                {changingPassword && !user.usingGoogleLogin && (
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            width: "100%",
                        }}
                    >
                        <Input
                            text="Current Password"
                            type="password"
                            placeholder="Current Password"
                            onChange={changePassword}
                            value={password}
                            warning={passwordWarning}
                        />
                        <PasswordConfirmForm
                            changePassword={changeNewPassword}
                            changeConfirmPassword={changeConfirmPassword}
                            password={newPassword}
                            confirmPassword={confirmPassword}
                            passwordWarning={newPasswordWarning}
                            confirmPasswordWarning={confirmPasswordWarning}
                            onKeyPress={() => {}}
                            profile
                        />
                        <div
                            style={{
                                display: "flex",
                                flexDirection: "row",
                                justifyContent: "space-between",
                            }}
                        >
                            <button
                                className="save-button"
                                onClick={savePassword}
                            >
                                {savingPassword ? (
                                    <FontAwesomeIcon
                                        icon={faCircleNotch}
                                        spin
                                        style={{
                                            color: "white",
                                        }}
                                    />
                                ) : (
                                    <span>SAVE</span>
                                )}
                            </button>
                            <button
                                className="white-button"
                                style={{
                                    width: "47%",
                                    padding: "15px 0px",
                                    fontSize: "16px",
                                    marginTop: "20px",
                                }}
                                onClick={() => setChangingPassword(false)}
                            >
                                CANCEL
                            </button>
                        </div>
                    </div>
                )}
                {!changingPassword && !user.usingGoogleLogin && (
                    <>
                        <div>••••••••••••</div>
                        <div
                            style={{
                                display: "flex",
                                flexDirection: "row",
                            }}
                        >
                            {savedPassword && (
                                <div className="saved">Saved!</div>
                            )}
                            <FaEdit
                                className="add-name"
                                onClick={() => setChangingPassword(true)}
                                style={{ fontSize: "20px" }}
                            />
                        </div>
                    </>
                )}
            </div>
        </>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(PasswordForm)
