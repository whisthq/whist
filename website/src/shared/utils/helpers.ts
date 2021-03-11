const crypto = require("crypto")

export const generateHashedPassword = (password: string): string => {
    /*
        Hash the user's plain-text password (to later use as encryption/decryption password for the config key)
        
        Arguments:
            password (string): plaintext password to hash
    */
    const token = crypto.pbkdf2Sync(
        password,
        process.env.SHA_SECRET_KEY,
        100000,
        32,
        "sha256"
    )
    return token.toString("hex")
}

export const generateConfigKey = async (): Promise<string> => {
    /*
        Generate a unique config key to encrypt/decrypt user configs with
 
        Arguments:
            none
    */
    const buffer: Buffer = await new Promise((resolve, reject) => {
        crypto.randomBytes(32, (err: Error, buf: Buffer) => {
            if (err) {
                reject(err)
            }
            resolve(buf)
        })
    })
    return buffer.toString("hex")
}

const algorithm = "aes-256-gcm"

export const encryptConfigKey = (text: string, password: string): string => {
    /*
        Encrypt the given config key with the given password

        Arguments:
            text (string): text to encrypt
            password (string): plaintext password to encrypt with
        
        Returns:
            (string): the encrypted text
    
    */

    const hashedPassword = generateHashedPassword(password)

    let cipher = crypto.createCipher(algorithm, hashedPassword)
    let crypted = cipher.update(text, "utf8", "hex")
    crypted += cipher.final("hex")
    return crypted
}

export const decryptConfigKey = (text: string, password: string): string => {
    /*
        Decrypt the given configc key with the given password

        Arguments:
            text (string): text to encrypt
            password (string): plaintext password to decrypt with
        
        Returns:
            (string): the decrypted text
    */
    const hashedPassword = generateHashedPassword(password)

    var decipher = crypto.createDecipher(algorithm, hashedPassword)
    var dec = decipher.update(text, "hex", "utf8")
    dec += decipher.final("utf8")
    return dec
}
