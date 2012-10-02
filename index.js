process.env['TERMINFO_DIRS'] = __dirname + '/deps/libncurses/terminfo';

var EventEmitter = require('events').EventEmitter,
    addon = require('./build/Release/binding');

addon.Window.prototype.__proto__ = EventEmitter.prototype;

module.exports = addon;
