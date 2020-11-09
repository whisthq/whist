const loadingMessages = [
    "Remember to save your files to cloud storage. Your cookies are erased at logoff to preserve your privacy.",
    "Fractal never tracks your streaming session and all streamed data is fully end-to-end encrypted.",
    "Join our Discord to chat with Fractal users and engineers: https://discord.gg/PDNpHjy.",
    "If you make cool artwork on Fractal, tweet us @tryfractal and we'll share it!",
    "Launch Fractal directly from any browser by typing fractal:// followed by a URL.",
    "You can zoom the Fractal window with Ctrl+/Cmd+ to adjust the resolution.",
]

export const generateMessage = (): string => {
    return _.shuffle(loadingMessages)[0]
}
