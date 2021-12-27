type WhistSender = "client" | "server" | "browser"
type WhistRecipient = "client" | "server" | "browser"

interface WhistMessage {
    name: string,
    from: WhistSender,
    to: WhistRecipient,
    sessionID: number,
    body?: object,
    onSend?: () => void,
    onReceive?: () => void
}

export { WhistSender, WhistRecipient, WhistMessage}