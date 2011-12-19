var EventEmitter = require('events').EventEmitter,
    addon = require('./ncurses_addon');

addon.Window.prototype.__proto__ = EventEmitter.prototype;

module.exports = addon;
