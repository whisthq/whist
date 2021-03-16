const crypto = require("crypto")

export const generateHashedPassword = (password: string): string => {
    /*
        Hash the user's plain-text password (to later use as encryption/decryption password for the config token)
        
        Arguments:
            password (string): plaintext password to hash
    */
    const token = crypto.pbkdf2Sync(
        password,
        process.env.SHA_SECRET_KEY,
        100000,
        16,
        "sha256"
    )
    return token.toString("hex")
}

export const generateRandomString = async (bytes: number) => {
    /*
        Generate a cryptographically secure random string
 
        Arguments:
            bytes (number): number of bytes in the string
    */
    const configToken: Buffer = await new Promise((resolve, reject) => {
        crypto.randomBytes(bytes, (err: Error, buf: Buffer) => {
            if (err) {
                reject(err)
            }
            resolve(buf)
        })
    })
    return configToken.toString("hex")
}

const algorithm = "aes-256-gcm"

export const encryptConfigToken = async (text: string, password: string) => {
    /*
        Encrypt the given config token with the given password

        Arguments:
            text (string): text to encrypt
            password (string): plaintext password to encrypt with
        
        Returns:
            (string): the encrypted text
    
    */
    const configIV = await generateRandomString(48)
    const hashedPassword = generateHashedPassword(password)

    var cipher = crypto.createCipheriv(algorithm, hashedPassword, configIV)
    var encrypted = cipher.update(text, "utf8", "hex")
    encrypted += cipher.final("hex")
    var tag = cipher.getAuthTag().toString("hex")

    return encrypted.concat(configIV, tag)
}

export const decryptConfigToken = (text: string, password: string): string => {
    /*
        Decrypt the given config token with the given password

        Arguments:
            text (string): text to decrypt
            password (string): plaintext password to decrypt with
        
        Returns:
            (string): the decrypted text
    */
    const hashedPassword = generateHashedPassword(password)
    const encrypted = text.substring(0, 128)
    const configIV = text.substring(128, 224)
    const tag = Buffer.from(text.substring(224), "hex")

    var decipher = crypto.createDecipheriv(algorithm, hashedPassword, configIV)
    decipher.setAuthTag(tag)
    var dec = decipher.update(encrypted, "hex", "utf8")
    dec += decipher.final("utf8")
    return dec
}
