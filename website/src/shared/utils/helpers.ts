export const generateEncryptionToken = (password: string) => {
    /*
        Generate an encryption key to send to the client-application
        
        Arguments:
            password (string): plaintext password to change into encryption key
    */

    const crypto = require("crypto")
    const token = crypto.pbkdf2Sync(
        password,
        "encryptionsecret",
        100000,
        32,
        "sha256"
    )
    return token.toString("hex")
}
