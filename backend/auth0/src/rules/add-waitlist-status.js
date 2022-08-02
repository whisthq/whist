/* add-waitlist-status.js
 *
 * Add the status of a user's waitlist status
 * For now, every new user is automatically added to the waitlist
 * 
 */

async function addWaitlistStatus(user, context, callback) {  
    context.accessToken["waitlisted"] = true;
    return callback(null, user, context);
}

  
