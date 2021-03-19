import React, { FC } from "react"
import classNames from "classnames"

export enum FractalWarningType {
    DEFAULT,
    SMALL,
}

interface BaseWarningProps {
    warning: string | null | undefined | JSX.Element
    className?: string
}

interface FractalWarningProps extends BaseWarningProps {
    type: FractalWarningType
}

export const BaseWarning: FC<BaseWarningProps> = (props: BaseWarningProps) => {
    const { warning } = props

    if (!warning) {
        return <></>
    } else {
        return (
            <div className={classNames("font-body rounded", props.className)}>
                {warning}
            </div>
        )
    }
}

export const FractalWarning: FC<FractalWarningProps> = (
    props: FractalWarningProps
) => {
    const { type, ...baseWarningProps } = props

    const smallClassName = classNames(
        props.className,
        "text-xs py-1 text-red text-right"
    )

    const defaultClassName = classNames(
        props.className,
        "w-full px-6 py-3 text-sm bg-red-100 text-red"
    )

    switch (type) {
        case FractalWarningType.SMALL:
            const smallWarningProps = Object.assign(baseWarningProps, {
                className: smallClassName,
            })
            return <BaseWarning {...smallWarningProps} />
        default:
            const defaultWarningProps = Object.assign(baseWarningProps, {
                className: defaultClassName,
            })
            return <BaseWarning {...defaultWarningProps} />
    }
}
