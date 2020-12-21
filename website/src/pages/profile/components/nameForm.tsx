import React, { useState } from "react"
import { connect } from "react-redux"
import { useMutation } from "@apollo/client"
import { FaEdit } from "react-icons/fa"

import Input from "shared/components/input"
import * as PureAuthAction from "store/actions/auth/pure"

import { UPDATE_NAME } from "pages/profile/constants/graphql"

import "styles/profile.css"

const NameForm = (props: any) => {
    const { dispatch, user } = props

    const [updateName] = useMutation(UPDATE_NAME, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
    })

    const [editingName, setEditingName] = useState(false)
    const [newName, setNewName] = useState(user.name)
    const [savedName, setSavedName] = useState(false)

    const changeNewName = (evt: any): any => {
        evt.persist()
        setNewName(evt.target.value)
    }

    // Update user's name in database and redux state
    const saveNewName = () => {
        setEditingName(false)
        updateName({
            variables: {
                user_id: user.user_id,
                name: newName,
            },
        })
        dispatch(PureAuthAction.updateUser({ name: newName }))
        setSavedName(true)
    }

    return (
        <>
            <div className="section-title">Name</div>
            <div className="section-info">
                {editingName ? (
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            width: "100%",
                        }}
                    >
                        <Input
                            type="name"
                            placeholder={"New name"}
                            onChange={changeNewName}
                            value={newName}
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
                                onClick={saveNewName}
                            >
                                SAVE
                            </button>
                            <button
                                className="white-button"
                                style={{
                                    width: "47%",
                                    padding: "15px 0px",
                                    fontSize: "16px",
                                    marginTop: "20px",
                                }}
                                onClick={() => setEditingName(false)}
                            >
                                CANCEL
                            </button>
                        </div>
                    </div>
                ) : (
                    <>
                        {user.name ? (
                            <div>{user.name} </div>
                        ) : (
                            <div
                                className="add-name"
                                onClick={() => {
                                    setEditingName(true)
                                    setSavedName(false)
                                }}
                            >
                                <FaEdit style={{ fontSize: "20px" }} />
                                <div
                                    style={{
                                        marginLeft: "15px",
                                        fontStyle: "italic",
                                    }}
                                >
                                    Add your name
                                </div>
                            </div>
                        )}
                        {user.name && (
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                }}
                            >
                                {savedName && (
                                    <div className="saved">Saved!</div>
                                )}
                                <FaEdit
                                    className="add-name"
                                    onClick={() => {
                                        setEditingName(true)
                                        setSavedName(false)
                                    }}
                                    style={{ fontSize: "20px" }}
                                />
                            </div>
                        )}
                    </>
                )}
            </div>
        </>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(NameForm)
