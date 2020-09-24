import React, { useState, useEffect } from 'react'
import { connect } from 'react-redux'
import styles from 'styles/login.css'
import Titlebar from 'react-electron-titlebar'
import Logo from 'assets/images/logo.svg'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import { fetchContainer } from 'store/actions/counter'

const UpdateScreen = (props: any) => {
    const { os, dispatch } = props

    const [percentageLeft, setPercentageLeft] = useState(300)
    const [percentageDownloaded, setPercentageDownloaded] = useState(0)
    const [downloadSpeed, setDownloadSpeed] = useState('0')
    const [downloadError, setDownloadError] = useState('')

    useEffect(() => {
        const ipc = require('electron').ipcRenderer

        dispatch(fetchContainer())

        ipc.on('percent', (_: any, percent: any) => {
            percent = percent * 3
            setPercentageLeft(300 - percent)
            setPercentageDownloaded(percent)
        })

        ipc.on('download-speed', (_: any, speed: any) => {
            setDownloadSpeed((speed / 1000000).toFixed(2))
        })

        ipc.on('transferred', (_: any, transferred: any) => {
            setTransferred((transferred / 1000000).toFixed(2))
        })

        ipc.on('total', (_: any, total: any) => {
            setTotal((total / 1000000).toFixed(2))
        })

        ipc.on('error', (_: any, error: any) => {
            setDownloadError(error)
        })
    }, [])

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
                                width: `${percentageDownloaded}px`,
                                height: 6,
                                background:
                                    'linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)',
                            }}
                        ></div>
                        <div
                            style={{
                                width: `${percentageLeft}px`,
                                height: 6,
                                background: '#111111',
                            }}
                        ></div>
                    </div>
                    {downloadError === '' ? (
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
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{
                                        color: '#5EC4EB',
                                        marginRight: 4,
                                        width: 12,
                                    }}
                                />{' '}
                                Downloading Update ({downloadSpeed} Mbps)
                            </div>
                        </div>
                    ) : (
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
                                {downloadError}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        os: state.counter.os,
    }
}

export default connect(mapStateToProps)(UpdateScreen)
