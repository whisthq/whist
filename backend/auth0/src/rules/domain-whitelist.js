/* domain-whitelist.js

   This rule will only allow access to users with specific email domains.
 */

function emailDomainWhitelist(user, context, callback) {
    // For the `dev` and `staging` environments, we only want Whist team members to
    // have access, for security purposes.
    if (context.tenant === 'fractal-dev' || context.tenant === 'fractal-staging') {
        // Access should only be granted to verified users
        if (!user.email || !user.email_verified) {
            return callback(new UnauthorizedError('Access denied.'));
        }

        const whitelist = ['whist.com']; // authorized domains
        const userHasAccess = whitelist.some(function (domain) {
            const emailSplit = user.email.split('@');
            return emailSplit[emailSplit.length - 1].toLowerCase() === domain;
        });

        if (!userHasAccess) {
            return callback(new UnauthorizedError('Access denied.'));
        }
    }
    
    return callback(null, user, context);
}
