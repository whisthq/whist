import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { useMutation } from "@apollo/client"
import { FaEdit } from "react-icons/fa"

import Input from "shared/components/input"
import * as PureAuthAction from "store/actions/auth/pure"

import { UPDATE_NAME } from "pages/profile/constants/graphql"

import "styles/profile.css"

const NameForm = (props: any) => {
    const { dispatch, user } = props

    const [completed, setCompleted] = useState(false)
    const [error, setError] = useState(false)

    const [updateName] = useMutation(UPDATE_NAME, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
        onCompleted: () => {
            setCompleted(true)
        },
        onError: () => {
            setError(true)
        },
    })

    const [editingName, setEditingName] = useState(false)
    const [newName, setNewName] = useState(user.name)
    const [savedName, setSavedName] = useState(false)

    const changeNewName = (evt: any): any => {
        evt.persist()
        setNewName(evt.target.value)
    }

    useEffect(() => {
        if (completed && !error) {
            setEditingName(false)
            dispatch(PureAuthAction.updateUser({ name: newName }))
            setSavedName(true)
        }
    }, [completed, error, dispatch, newName])

    // Update user's name in database and redux state
    const saveNewName = () => {
        setCompleted(false)
        setError(false)
        updateName({
            variables: {
                userID: user.userID,
                name: newName,
            },
        })
    }

    const handleCancel = () => {
        setCompleted(false)
        setError(false)
        setEditingName(false)
    }

    return (
        <>
            <div className="section-title">
Name
</div>
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
                            warning={error ? "Unable to save name." : ""}
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
                                onClick={handleCancel}
                            >
                                CANCEL
                            </button>
                        </div>
                    </div>
                ) : (
                    <>
                        {user.name ? (
                            <div>
{user.name}
{' '}
 </div>
                        ) : (
                            <div
                                className="add"
                                onClick={() => {
                                    setEditingName(true)
                                    setSavedName(false)
                                }}
                            >
                                + Add your name
                            </div>
                        )}
                        {user.name && (
                            <div
                                style={{
                                    position: "relative",
                                    display: "flex",
                                    flexDirection: "row",
                                }}
                            >
                                {savedName && (
                                    <div className="saved">
Saved!
</div>
                                )}
                                <FaEdit
                                    className="edit"
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
