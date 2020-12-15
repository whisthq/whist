/* eslint-disable import/no-named-as-default */

import React from "react"
import { Row } from "react-bootstrap"
import { connect } from "react-redux"

import Fractal from "assets/images/fractal.svg"

import AdminDropdown from "pages/dashboard/components/admin/adminDropdown"
import {
    FractalRegions,
    FractalWebservers,
    FractalTaskDefs,
    FractalClusters,
} from "pages/dashboard/components/admin/dropdownValues"

import { updateAdmin } from "store/actions/pure"
import { DEFAULT } from "store/reducers/states"
import { Dispatch } from "store/reducers/types"

export const AdminSettings = (props: {
    dispatch: Dispatch
    webserverUrl: string
    taskArn: string
    region: string
    cluster: string
}) => {
    const { dispatch, webserverUrl, taskArn, region, cluster } = props

    const handleSaveWebserver = (value: any) => {
        dispatch(
            updateAdmin({
                webserverUrl: value,
            })
        )
    }

    const handleSaveRegion = (value: any) => {
        dispatch(
            updateAdmin({
                region: value,
            })
        )
    }

    const handleSaveTask = (value: any) => {
        dispatch(
            updateAdmin({
                taskArn: value,
            })
        )
    }

    const handleSaveCluster = (value: any) => {
        dispatch(
            updateAdmin({
                cluster: value,
            })
        )
    }
    return (
        <div>
            <Row
                style={{
                    display: "flex",
                    flexDirection: "column",
                    marginTop: 50,
                    marginBottom: 25,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 16,
                            fontWeight: "bold",
                        }}
                    >
                        <img
                            alt="Test app"
                            src={Fractal}
                            style={{
                                color: "#111111",
                                height: 14,
                                marginRight: 12,
                                position: "relative",
                                top: 2,
                                width: 16,
                            }}
                        />
                        (Admin Only) Test Settings
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: "#333333",
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        Give a region (like us-east-1) and a webserver version
                        (local, dev, staging, prod, or a url) as well as a task
                        def and a cluster if you want. Defaults are provided to
                        us-east-1, dev, fractal-broswers-chrome, and null
                        cluster.
                    </div>
                </div>
                <div
                    style={{
                        width: "25%",
                        display: "flex",
                        flexDirection: "column",
                        justifyContent: "space-around",
                        alignItems: "flex-end",
                        padding: "10px 0px",
                    }}
                >
                    <div style={{ float: "right" }}>
                        <AdminDropdown
                            onClick={handleSaveWebserver}
                            options={FractalWebservers}
                            title="Webserver"
                            defaultValue={DEFAULT.admin.webserverUrl}
                            value={webserverUrl}
                        />
                        <AdminDropdown
                            onClick={handleSaveRegion}
                            options={FractalRegions}
                            title="Region"
                            defaultValue={DEFAULT.admin.region}
                            value={region}
                        />
                        <AdminDropdown
                            onClick={handleSaveTask}
                            options={FractalTaskDefs}
                            title="Task"
                            defaultValue={DEFAULT.admin.taskArn}
                            value={taskArn}
                        />
                        <AdminDropdown
                            onClick={handleSaveCluster}
                            options={FractalClusters}
                            title="Cluster"
                            defaultValue={DEFAULT.admin.cluster}
                            value={cluster}
                        />
                    </div>
                </div>
            </Row>
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        admin: {
            webserverUrl: string
            taskArn: string
            region: string
            cluster: string
        }
    }
}) => {
    return {
        webserverUrl: state.MainReducer.admin.webserverUrl,
        taskArn: state.MainReducer.admin.taskArn,
        region: state.MainReducer.admin.region,
        cluster: state.MainReducer.admin.cluster,
    }
}

export default connect(mapStateToProps)(AdminSettings)
