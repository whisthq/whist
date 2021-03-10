import classNames from "classnames"


export const withClass = <T extends Function>(Element: T, ...classes: string[]) => (
    props: any
) => <Element {...props} className={classNames(...classes, props.className)} />
