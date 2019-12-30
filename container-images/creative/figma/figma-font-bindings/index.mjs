import addon from './native/index.node';

let dirs = [
    `${process.env.HOME}/.local/share/bad_fonts`,
    `${process.env.HOME}/.local/share/fonts`,
    '/usr/share/fonts'
];

addon.getFonts(dirs, (err, result) => {
    if (err) {
        console.error('ERROR: ', err);
    }

    console.log('result: ', result);
});


console.log('last log');
