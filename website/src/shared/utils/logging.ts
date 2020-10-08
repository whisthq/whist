DEBUG = false; // set to false to disable debugging
function debug_log() {
    if ( DEBUG ) {
        console.log.apply(this, arguments);
    } 
    //DO NOTHING IF NOT 
}
