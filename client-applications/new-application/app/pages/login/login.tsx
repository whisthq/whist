import React, { useState, useEffect } from 'react'
import { connect } from 'react-redux'
// import { history } from "store/configureStore";
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import { parse } from 'url'
import Background from 'assets/images/background.jpg'
import Logo from 'assets/images/logo.svg'
import UpdateScreen from 'pages/dashboard/components/update'

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import {
    faCircleNotch,
    faUser,
    faLock,
} from '@fortawesome/free-solid-svg-icons'

import { FaGoogle } from 'react-icons/fa'

import { loginUser, setOS, loginFailed, googleLogin } from 'store/actions/main'

import { GOOGLE_CLIENT_ID, GOOGLE_REDIRECT_URI } from 'constants/config'

// import "styles/login.css";

const Login = (props: any) => {
    const { dispatch, os, warning } = props

    const [username, setUsername] = useState('')
    const [password, setPassword] = useState('')
    const [loggingIn, setLoggingIn] = useState(false)
    const [version, setVersion] = useState('1.0.0')
    const [rememberMe, setRememberMe] = useState(false)
    const live = useState(true)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [needsAutoupdate, setNeedsAutoupdate] = useState(false)
    const [fetchedCredentials, setFetchedCredentials] = useState(false)

    const storage = require('electron-json-storage')

    const updateUsername = (evt: any) => {
        setUsername(evt.target.value)
    }

    const updatePassword = (evt: any) => {
        setPassword(evt.target.value)
    }

    const handleLoginUser = () => {
        dispatch(loginFailed(false))
        setLoggingIn(true)
        if (rememberMe) {
            storage.set('credentials', {
                username: username,
                password: password,
            })
        } else {
            storage.set('credentials', { username: '', password: '' })
        }
        setUsername(username)
        setPassword(password)
        dispatch(loginUser(username.trim(), password))
    }

    const loginKeyPress = (event: any) => {
        if (event.key === 'Enter') {
            handleLoginUser()
        }
    }

    const handleGoogleLogin = () => {
        const { BrowserWindow } = require('electron').remote

        const authWindow = new BrowserWindow({
            width: 800,
            height: 600,
            show: false,
            'node-integration': false,
            'web-security': false,
        })
        const authUrl = `https://accounts.google.com/o/oauth2/v2/auth?scope=openid%20profile%20email&openid.realm&include_granted_scopes=true&response_type=code&redirect_uri=${GOOGLE_REDIRECT_URI}&client_id=${GOOGLE_CLIENT_ID}`
        authWindow.loadURL(authUrl, { userAgent: 'Chrome' })
        authWindow.show()

        const handleNavigation = (url: any) => {
            const query = parse(url, true).query
            if (query) {
                if (query.error) {
                    dispatch(loginFailed(true))
                } else if (query.code) {
                    authWindow.removeAllListeners('closed')
                    setImmediate(() => authWindow.close())
                    setLoggingIn(true)
                    dispatch(googleLogin(query.code))
                }
            }
        }

        authWindow.webContents.on('will-navigate', (_: any, url: any) => {
            handleNavigation(url)
        })

        authWindow.webContents.on(
            'did-get-redirect-request',
            (_: any, oldUrl: any, newUrl: any) => {
                handleNavigation(newUrl)
            }
        )
    }

    const forgotPassword = () => {
        const { shell } = require('electron')
        shell.openExternal('https://www.fractalcomputers.com/reset')
    }

    const signUp = () => {
        const { shell } = require('electron')
        shell.openExternal('https://www.fractalcomputers.com/auth')
    }

    const changeRememberMe = (event: any) => {
        setRememberMe(event.target.checked)
    }

    useEffect(() => {
        console.log('username')
        console.log(username)
        console.log(loggingIn)
        const ipc = require('electron').ipcRenderer
        const storage = require('electron-json-storage')

        ipc.on('update', (_: any, update: any) => {
            console.log('received update')
            console.log(update)
            setUpdatePingReceived(true)
            setNeedsAutoupdate(update)
        })

        const appVersion = require('../../package.json').version
        const os = require('os')
        dispatch(setOS(os.platform()))
        setVersion(appVersion)

        storage.get('credentials', (error: any, data: any) => {
            if (error) throw error

            if (data && Object.keys(data).length > 0) {
                if (data.username && data.password && live) {
                    setUsername(data.username)
                    setPassword(data.password)
                    setLoggingIn(true)
                    setFetchedCredentials(true)
                    console.log('set loggingin to true')
                }
            }
        })

        // if (username && publicIP && live) {
        //     history.push("/dashboard");
        // }
    }, [])

    useEffect(() => {
        console.log('in useeffect2')
        console.log(updatePingReceived)
        console.log(fetchedCredentials)
        if (
            updatePingReceived &&
            fetchedCredentials &&
            !needsAutoupdate &&
            username &&
            password
        ) {
            dispatch(loginUser(username, password))
        }
    }, [updatePingReceived, fetchedCredentials])

    return (
        <div
            className={styles.container}
            data-tid="container"
            style={{ backgroundImage: `url(${Background})` }}
        >
            <UpdateScreen />
            <div
                style={{
                    position: 'absolute',
                    bottom: 15,
                    right: 15,
                    fontSize: 11,
                    color: '#D1D1D1',
                }}
            >
                Version: {version}
            </div>
            {os === 'win32' ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
            {live ? (
                <div className={styles.removeDrag}>
                    <div className={styles.landingHeader}>
                        <div className={styles.landingHeaderLeft}>
                            <img src={Logo} width="18" height="18" />
                            <span className={styles.logoTitle}>Fractal</span>
                        </div>
                        <div className={styles.landingHeaderRight}>
                            <span id="forgotButton" onClick={forgotPassword}>
                                Forgot Password?
                            </span>
                            <button
                                type="button"
                                className={styles.signupButton}
                                style={{ borderRadius: 5, marginLeft: 15 }}
                                id="signup-button"
                                onClick={signUp}
                            >
                                Sign Up
                            </button>
                        </div>
                    </div>
                    <div style={{ marginTop: warning ? 10 : 60 }}>
                        <div className={styles.loginContainer}>
                            <div>
                                <FontAwesomeIcon
                                    icon={faUser}
                                    style={{
                                        color: 'white',
                                        fontSize: 12,
                                    }}
                                    className={styles.inputIcon}
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updateUsername}
                                    type="text"
                                    className={styles.inputBox}
                                    style={{ borderRadius: 5 }}
                                    placeholder={
                                        username ? username : 'Username'
                                    }
                                    id="username"
                                />
                            </div>
                            <div>
                                <FontAwesomeIcon
                                    icon={faLock}
                                    style={{
                                        color: 'white',
                                        fontSize: 12,
                                    }}
                                    className={styles.inputIcon}
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updatePassword}
                                    type="password"
                                    className={styles.inputBox}
                                    style={{ borderRadius: 5 }}
                                    placeholder={
                                        password ? '•••••••••' : 'Password'
                                    }
                                    id="password"
                                />
                            </div>
                            <div style={{ marginBottom: 20 }}>
                                {loggingIn && !warning ? (
                                    <button
                                        type="button"
                                        className={styles.loginButton}
                                        id="login-button"
                                        style={{
                                            opacity: 0.6,
                                            textAlign: 'center',
                                        }}
                                    >
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: 'white',
                                                width: 12,
                                                marginRight: 5,
                                                position: 'relative',
                                                top: 0.5,
                                            }}
                                        />{' '}
                                        Processing
                                    </button>
                                ) : (
                                    <button
                                        onClick={handleLoginUser}
                                        type="button"
                                        className={styles.loginButton}
                                        id="login-button"
                                    >
                                        START
                                    </button>
                                )}
                                <div style={{ marginBottom: 20 }}>
                                    {loggingIn && !warning ? (
                                        <button
                                            type="button"
                                            className={styles.googleButton}
                                            id="google-button"
                                            style={{
                                                opacity: 0.6,
                                                textAlign: 'center',
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faCircleNotch}
                                                spin
                                                style={{
                                                    color: 'white',
                                                    width: 12,
                                                    marginRight: 5,
                                                    position: 'relative',
                                                    top: 0.5,
                                                }}
                                            />{' '}
                                            Processing
                                        </button>
                                    ) : (
                                        <button
                                            onClick={handleGoogleLogin}
                                            type="button"
                                            className={styles.googleButton}
                                            id="google-button"
                                        >
                                            <FaGoogle
                                                style={{
                                                    fontSize: 16,
                                                    marginRight: 10,
                                                    position: 'relative',
                                                    top: 3,
                                                }}
                                            />
                                            Login with Google
                                        </button>
                                    )}
                                </div>
                            </div>
                            {warning && (
                                <div
                                    style={{
                                        textAlign: 'center',
                                        fontSize: 12,
                                        color: '#f9000b',
                                        background: 'rgba(253, 240, 241, 0.9)',
                                        width: '100%',
                                        padding: 15,
                                        borderRadius: 2,
                                        margin: 'auto',
                                        marginBottom: 30,
                                        // width: 265,
                                    }}
                                >
                                    <div>
                                        Invalid credentials. If you lost your
                                        password, you can reset it on the&nbsp;
                                        <div
                                            onClick={forgotPassword}
                                            className={styles.pointerOnHover}
                                            style={{
                                                display: 'inline',
                                                fontWeight: 'bold',
                                                textDecoration: 'underline',
                                            }}
                                        >
                                            website
                                        </div>
                                        .
                                    </div>
                                </div>
                            )}
                            <div
                                style={{
                                    marginTop: 25,
                                    display: 'flex',
                                    justifyContent: 'center',
                                    alignItems: 'center',
                                }}
                            >
                                <label className={styles.termsContainer}>
                                    <input
                                        type="checkbox"
                                        onChange={changeRememberMe}
                                        onKeyPress={loginKeyPress}
                                    />
                                    <span className={styles.checkmark} />
                                </label>

                                <div style={{ fontSize: 12 }}>Remember Me</div>
                            </div>
                        </div>
                    </div>
                </div>
            ) : (
                <div
                    style={{
                        lineHeight: 1.5,
                        margin: '150px auto',
                        maxWidth: 400,
                    }}
                >
                    {' '}
                    We are currently pushing out a critical Linux update. Your
                    app will be back online very soon. We apologize for the
                    inconvenience!
                </div>
            )}
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        username: state.MainReducer.username,
        loginWarning: state.MainReducer.loginWarning,
        os: state.MainReducer.os,
    }
}

export default connect(mapStateToProps)(Login)
