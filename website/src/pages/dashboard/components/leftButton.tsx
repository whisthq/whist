import React from "react"
import { Link } from "react-router-dom"
import { FaHome, FaCog } from "react-icons/fa"
import { Tooltip, OverlayTrigger } from "react-bootstrap"

import { LeftButtonType } from "shared/types/ui"

export const LeftButton = (props: {
    exact: boolean
    path: string
    iconName: LeftButtonType
    id?: string
}) => {
    /*
        Icon button on left-hand side of dashboard that lets user navigate the
        sub-pages of the dashboard (e.g. home, settings)
 
        Arguments:
            path (string): Where the button should redirect to (e.g. /dashboard) 
            exact (boolean): If the button should be highlighted when you are on the exact
                path or just a path that starts with path
            iconName (LeftButtonType): Icon that appears on the button    
    */
    const { path, exact, iconName, id } = props

    const matchPath = () => {
        if (exact) {
            return window.location.pathname === path
        } else {
            return window.location.pathname.startsWith(path)
        }
    }

    const LeftButtonIcon = {
        Home: (
            <>
                {matchPath() ? (
                    <>
                        <FaHome className="absolute top-1/2 left-1/2 transform -translate-y-1/2 -translate-x-1/2 text-white text-lg" />
                    </>
                ) : (
                    <>
                        <FaHome className="absolute top-1/2 left-1/2 transform -translate-y-1/2 -translate-x-1/2 text-blue-300 text-lg" />
                    </>
                )}
            </>
        ),
        Settings: (
            <>
                {matchPath() ? (
                    <>
                        <FaCog className="absolute top-1/2 left-1/2 transform -translate-y-1/2 -translate-x-1/2 text-white text-lg" />
                    </>
                ) : (
                    <>
                        <FaCog className="absolute top-1/2 left-1/2 transform -translate-y-1/2 -translate-x-1/2 text-blue-300 text-lg" />
                    </>
                )}
            </>
        ),
    }

    return (
        <>
            <OverlayTrigger
                placement="right"
                overlay={
                    <Tooltip id="button-tooltip">
                        <div>{iconName}</div>
                    </Tooltip>
                }
            >
                <Link to={path} id={id}>
                    {matchPath() ? (
                        <div className="w-12 h-12 mb-8 rounded relative bg-blue shadow-xl cursor-pointer duration-500">
                            {LeftButtonIcon[iconName]}
                        </div>
                    ) : (
                        <div className="w-12 h-12 mb-8 rounded relative bg-blue-100 shadow-xl cursor-pointer duration-500">
                            {LeftButtonIcon[iconName]}
                        </div>
                    )}
                </Link>
            </OverlayTrigger>
        </>
    )
}

export default LeftButton
