declare module "*.svg" {
    const content: any
    export default content
}

declare module "*.jpg" {
    const content: string
    export default content
}

declare module "*.png" {
    const content: string
    export default content
}

declare module "*.json" {
    const content: string
    export default content
}

declare module "*.gif" {
    const content: string
    export default content
}

declare module "*.css" {
    interface IClassNames {
        [className: string]: string
    }
    const classNames: IClassNames
    export = classNames
}

declare module "react-typeform-embed"
