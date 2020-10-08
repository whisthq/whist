var debug_log = function debug_log(callback:any){
    if(process.env.NODE_ENV === 'development'){
        console.log(callback);
    }
}

export {}
