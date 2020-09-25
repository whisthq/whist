import React, { useState, useEffect, useCallback } from 'react'
import { connect } from 'react-redux'
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import Logo from 'assets/images/logo.svg'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import { fetchContainer } from 'store/actions/counter'

const UpdateScreen = (props: any) => {
    const { os, dispatch, percentLoaded, status } = props

    const [percentLoadedWidth, setPercentLoadedWidth] = useState(0)
    const [percentLeftWidth, setPercentLeftWidth] = useState(300)

    const [launched, setLaunched] = useState(false)

    useEffect(() => {
        dispatch(fetchContainer())
    }, [])

    useEffect(() => {
        console.log(percentLoadedWidth < percentLoaded * 3)
        if (percentLoadedWidth < percentLoaded * 3) {
            const newWidth = percentLoadedWidth + 2
            setPercentLoadedWidth(newWidth)
            setPercentLeftWidth(300 - newWidth)
        }
        // launch if we are done loading
        if (percentLoaded >= 100 && !launched) {
            setLaunched(true)

            console.log('signal launch!')
            var child = require('child_process').spawn
            var appRootDir = require('electron').remote.app.getAppPath()
            var executable = ''
            var path = ''

            const os = require('os')

            if (os.platform() === 'darwin') {
                console.log('darwin found')
                path = appRootDir + '/protocol-build/desktop/'
                //path = path.replace('/Resources/app.asar', '')
                //path = path.replace('/desktop/app', '/desktop')
                executable = './FractalClient'
            } else {
                console.log('darwin not found, found ' + os.platform())
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
                //'-p',
                //mystery,
                ip,
            ]
            console.log('params set gonna try now on path (cwd) ' + path)
            //console.log('PATH is ' + process.env.PATH)

            // Starts the protocol
            const protocol1 = child(executable, parameters, {
                cwd: path,
                detached: true,
                stdio: 'ignore',
                env: {
                    PATH: process.env.PATH,
                }, // https://maxschmitt.me/posts/error-spawn-node-enoent-node-js-child-process/
            })
            /*
            
            */
            protocol1.on('close', (code: any) => {
                console.log('closed the protocol')
            })

            console.log('child created!')
        }
    }, [launched, percentLoaded, percentLoadedWidth])

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
        // todo perhaps add launched
    }
}

export default connect(mapStateToProps)(UpdateScreen)