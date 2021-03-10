import React, { useState, useEffect, ChangeEvent, Dispatch } from "react"
import { connect } from "react-redux"
import { FaPencilAlt } from "react-icons/fa"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import Input from "shared/components/input"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import { User, AuthFlow } from "shared/types/reducers"
import * as PureAuthAction from "store/actions/auth/pure"
import { updatePassword } from "store/actions/auth/sideEffects"
import { checkPasswordVerbose } from "pages/auth/constants/authHelpers"
import classNames from "classnames"

import { E2E_DASHBOARD_IDS, PROFILE_IDS } from "testing/utils/testIDs"

import styles from "styles/profile.module.css"
import sharedStyles from "styles/shared.module.css"
import PLACEHOLDER from "shared/constants/form"

const PasswordForm = (props: {
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
}) => {
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
    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
        if (passwordWarning !== "") {
            setPasswordWarning("")
        }
    }

    const changeNewPassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setNewPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: ChangeEvent<HTMLInputElement>) => {
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
            dispatch(
                PureAuthAction.updateAuthFlow({
                    resetDone: false,
                    passwordVerified: null,
                })
            )
        } else if (authFlow.passwordVerified === "failed") {
            setPasswordWarning("Incorrect current password")
            setSavingPassword(false)
        }
    }, [
        authFlow.passwordVerified,
        authFlow.resetDone,
        changingPassword,
        dispatch,
    ])

    return (
        <div style={{ marginTop: 15 }} data-testid={PROFILE_IDS.PASSWORD}>
            <div className={classNames("font-body", styles.sectionTitle)}>Password</div>
            <div className={styles.sectionInfo}>
                {user.usingGoogleLogin && (
                    <div className="font-body" style={{ fontStyle: "italic" }}>
                        Logged in through Google
                    </div>
                )}
                {changingPassword && !user.usingGoogleLogin && (
                    <div
                        className={styles.dashedBox}
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            width: "100%",
                            padding: "15px 20px",
                        }}
                    >
                        <Input
                            id={E2E_DASHBOARD_IDS.CURRENTPASSWORD}
                            text="Current Password"
                            type="password"
                            placeholder={PLACEHOLDER.CURRENTPASSWORD}
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
                                className={styles.saveButton}
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
                                    <span className="font-body">SAVE</span>
                                )}
                            </button>
                            <button
                                className={classNames("font-body", sharedStyles.whiteButton)}
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
                    <div
                        className={styles.dashedBox}
                        style={{
                            display: "flex",
                            justifyContent: "space-between",
                        }}
                    >
                        <div className="font-body">••••••••••••</div>
                        <div
                            style={{
                                display: "flex",
                                flexDirection: "row",
                            }}
                        >
                            {savedPassword && (
                                <div className={styles.saved}>Saved!</div>
                            )}
                            <div data-testid={PROFILE_IDS.EDITICON}>
                                <FaPencilAlt
                                    id={E2E_DASHBOARD_IDS.EDITPASSWORD}
                                    className="relative top-1 cursor-pointer"
                                    onClick={() => setChangingPassword(true)}
                                    style={{ position: "relative", top: 5 }}
                                />
                            </div>
                        </div>
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(PasswordForm)
