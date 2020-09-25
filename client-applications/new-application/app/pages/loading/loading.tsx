import React, { useState, useEffect } from 'react'
import { connect } from 'react-redux'
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import Logo from 'assets/images/logo.svg'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import { fetchContainer, attachDisk, changeStatusMessage, sendLogs, askFeedback } from 'store/actions/counter'

const UpdateScreen = (props: any) => {
    const { os, dispatch, percentLoaded, status } = props

    const [percentLoadedWidth, setPercentLoadedWidth] = useState(0)
    const [percentLeftWidth, setPercentLeftWidth] = useState(300)

    // TODO SHOULD THIS EVEN BE HERE?

    const [launches, setLaunches] = useState(0)
    const [reattached, setReattached] = useState(false)
    const [launched, setLaunched] = useState(false)
    const [diskAttaching, setDiskAttaching] = useState(false)

    // TODO SHOULD THIS EVEN BE IN THIS FILE?!?!?
    const ParseLogs = (upload: any) => {
        const fs = require('fs')
        const os = require('os')

        var logs = ''
        var connection_id = 0
        var log_path = ''
        var connection_id_path = ''

        try {
            if (os.platform() === 'darwin') {
                connection_id_path =
                    process.env.HOME + '/.fractal/connection_id.txt'
                log_path = process.env.HOME + '/.fractal/log.txt'
            }

            connection_id = parseInt(
                fs.readFileSync(connection_id_path).toString()
            )

            logs = fs.readFileSync(log_path, 'utf-8')

            let line_by_line = logs.split('\n')

            line_by_line = line_by_line.slice(
                Math.max(0, line_by_line.length - 50),
                line_by_line.length
            )

            line_by_line = line_by_line.filter(function (line) {
                return (
                    line.includes('Server signaled a quit!') ||
                    line.includes('Forcefully Quitting')
                )
            })

            const graceful_exit_detected = line_by_line.length > 0

            if (graceful_exit_detected || upload) {
                props.dispatch(sendLogs(connection_id, logs))
            }
            return graceful_exit_detected
        } catch (err) {
            console.log('Log Error: ' + err.toString())
        }
    }

    // TODO SHOULD THIS EVEN BE IN THIS FILE?!?!?
    const LaunchProtocol = () => {
        if (reattached) {
            setLaunched(true)
            props.dispatch(
                changeStatusMessage(
                    'Boot request sent to server. Waiting for a response.'
                )
            )
            if (launches == 0) {
                setLaunches(1)
                setReattached(false)
            }
        } else {
            // TODO ?!?!?!
            setLaunched(false)
            setDiskAttaching(true)
            props.dispatch(attachDisk())
        }
    }

    // effects below, helpers and constants above

    useEffect(() => {
        dispatch(fetchContainer())
    }, [])

    useEffect(() => {
        if (percentLoadedWidth < percentLoaded * 3) {
            const newWidth = percentLoadedWidth + 2
            setPercentLoadedWidth(newWidth)
            setPercentLeftWidth(300 - newWidth)
        }
    }, [percentLoaded, percentLoadedWidth])

    // TODO this should be running onChange only for launches -> 1, reattached -> false
    // maybe create a new state variable?
    useEffect(() => {
        var child = require('child_process').spawn
        var appRootDir = require('electron').remote.app.getAppPath()
        var executable = ''
        var path = ''

        const os = require('os')

        if (os.platform() === 'darwin') {
            console.log('yeet')
            path = appRootDir + '/protocol-build/desktop/'
            path = path.replace('/Resources/app.asar', '')
            path = path.replace('/desktop/app', '/desktop')
            executable = './FractalClient'
        }

        var screenWidth = 200
        var screenHeight = 200
        var mystery = '32262:32780,32263:32778,32273:32779'
        var ip = '34.206.64.200'

        // ./desktop -w200 -h200 -p32262:32780,32263:32778,32273:32779 34.206.64.200
        var parameters = [
            '-w',
            screenWidth,
            '-h',
            screenHeight,
            '-p',
            mystery,
            ip,
        ]

        // Starts the protocol
        const protocol1 = child(executable, parameters, {
            cwd: path,
            detached: true,
            stdio: 'ignore',
        })

        //Listener for closing the stream window
        protocol1.on('close', (code: any) => {
            let graceful_exit_detected = ParseLogs(false)
            if (graceful_exit_detected) {
                setLaunches(0)
                setLaunched(false)
                setReattached(false)
                setDiskAttaching(false)

                props.dispatch(askFeedback(true))
            } else {
                const protocol2 = child(executable, parameters, {
                    cwd: path,
                    detached: true,
                    stdio: 'ignore',
                })

                protocol2.on('close', (code: any) => {
                    ParseLogs(true)

                    // TODO CHECK SCOPE (i.e. comparing with the MainBox in desktop)
                    // there may a reason everything was component. and not this.
                    setLaunches(0)
                    setLaunched(false)
                    setReattached(false)
                    setDiskAttaching(false)

                    props.dispatch(askFeedback(true))
                })
            }
        })
    }, [launches, reattached])

    return (
        <div
            style={{
                position: 'fixed',
                top: 0,
                left: 0,
                width: 1000,
                height: 680,
                backgroundColor: '#0B172B',
                zIndex: 1000,
            }}
        >
            {os === 'win32' ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div style={{ marginTop: 10 }}></div>
            )}
            <div className={styles.landingHeader}>
                <div className={styles.landingHeaderLeft}>
                    <img src={Logo} width="20" height="20" />
                    <span className={styles.logoTitle}>Fractal</span>
                </div>
            </div>
            <div style={{ position: 'relative' }}>
                <div
                    style={{
                        paddingTop: 180,
                        textAlign: 'center',
                        color: 'white',
                        width: 1000,
                    }}
                >
                    <div
                        style={{
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                        }}
                    >
                        <div
                            style={{
                                width: `${percentLoadedWidth}px`,
                                height: 6,
                                background:
                                    'linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)',
                            }}
                        ></div>
                        <div
                            style={{
                                width: `${percentLeftWidth}px`,
                                height: 6,
                                background: '#111111',
                            }}
                        ></div>
                    </div>
                    <div
                        style={{
                            marginTop: 10,
                            fontSize: 14,
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                        }}
                    >
                        <div style={{ color: '#D6D6D6' }}>
                            {status != 'SUCCESS.' && (
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{
                                        color: '#5EC4EB',
                                        marginRight: 4,
                                        width: 12,
                                    }}
                                />
                            )}{' '}
                            {status}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        os: state.counter.os,
        percentLoaded: state.counter.percent_loaded,
        status: state.counter.status_message,
        launches: state.launches, // TODO IS THIS SUPPOSED TO BE HERE?!
        reattached: state.reattached //TODO
    }
}

export default connect(mapStateToProps)(UpdateScreen)
