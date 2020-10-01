import React, { useState, useEffect, useCallback } from 'react'
import { connect } from 'react-redux'
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import Logo from 'assets/images/logo.svg'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import { fetchContainer } from 'store/actions/main'

const UpdateScreen = (props: any) => {
    const {
        os,
        dispatch,
        percentLoaded,
        status,
        container_id,
        port_32262,
        port_32263,
        port_32273,
        width, // for the screen
        height, // for the screen
        ip,
        codec,
    } = props

    // figure out how to use useEffect
    // note to future developers: setting state inside useffect when you rely on
    // change for those variables to trigger runs forever and is bad
    // use two variables for that or instead do something like this below
    var percentLoadedWidth = 3 * percentLoaded
    var percentLeftWidth = 300 - 3 * percentLoaded

    useEffect(() => {
        dispatch(fetchContainer())
    }, [])

    useEffect(() => {
        if (container_id) {
            LaunchProtocol()
        }
    }, [container_id])

    const LaunchProtocol = () => {
        var child = require('child_process').spawn
        var appRootDir = require('electron').remote.app.getAppPath()
        var executable = ''
        var path = ''

        const os = require('os')

        if (os.platform() === 'darwin') {
            console.log('darwin found')
            path = appRootDir + '/protocol-build/desktop/'
            path = path.replace('/app', '')
            executable = './FractalClient'
        } else if (os.platform() === 'linux') {
            console.log('linux found')
            path = process.cwd() + '/protocol-build'
            path = path.replace('/release', '')
            executable = './FractalClient'
        } else if (os.platform() === 'win32') {
            console.log('windows found')
            path = process.cwd() + '\\protocol-build\\desktop'
            executable = 'FractalClient.exe'
        } else {
            console.log(`no suitable os found, instead got ${os.platform()}`)
        }

        var port_info = `32262:${port_32262},32263:${port_32263},32273:${port_32273}`
        var parameters = [
            '-w',
            width,
            '-h',
            height,
            '-p',
            port_info,
            '-c',
            codec,
            ip,
        ]
        console.log(`your executable path should be: ${path}`)

        // Starts the protocol
        const protocol1 = child(executable, parameters, {
            cwd: path,
            detached: true,
            stdio: 'ignore',
            // optional:
            //env: {
            //    PATH: process.env.PATH,
            //},
        })
        protocol1.on('close', (code: any) => {
            console.log('the protocol has been closed!')
        })
        console.log('spawn completed!')

        // TODO (adriano) graceful exit vs non graceful exit code
        // this should be done AFTER the endpoint to connect to EXISTS
    }

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
                            {status != 'Successfully created container.' &&
                                status != 'Successfully deleted container.' && (
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
        os: state.MainReducer.os,
        percentLoaded: state.MainReducer.percent_loaded,
        status: state.MainReducer.status_message,
        container_id: state.MainReducer.container_id,
        cluster: state.MainReducer.cluster,
        port_32262: state.MainReducer.port_32262,
        port_32263: state.MainReducer.port_32263,
        port_32273: state.MainReducer.port_32273,
        location: state.MainReducer.location,
        ip: state.MainReducer.ip,
        codec: state.MainReducer.codec,
    }
}

export default connect(mapStateToProps)(UpdateScreen)
