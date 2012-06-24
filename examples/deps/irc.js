/*
    irc.js - Node JS IRC client library

    (C) Copyright Martyn Smith 2010

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

exports.Client = Client;

var util = require('util');
var net = require('net');

const replyFor = { // {{{
   "200" : {
      "name" : "rpl_tracelink",
      "type" : "reply"
   },
   "201" : {
      "name" : "rpl_traceconnecting",
      "type" : "reply"
   },
   "202" : {
      "name" : "rpl_tracehandshake",
      "type" : "reply"
   },
   "203" : {
      "name" : "rpl_traceunknown",
      "type" : "reply"
   },
   "204" : {
      "name" : "rpl_traceoperator",
      "type" : "reply"
   },
   "205" : {
      "name" : "rpl_traceuser",
      "type" : "reply"
   },
   "206" : {
      "name" : "rpl_traceserver",
      "type" : "reply"
   },
   "208" : {
      "name" : "rpl_tracenewtype",
      "type" : "reply"
   },
   "211" : {
      "name" : "rpl_statslinkinfo",
      "type" : "reply"
   },
   "212" : {
      "name" : "rpl_statscommands",
      "type" : "reply"
   },
   "213" : {
      "name" : "rpl_statscline",
      "type" : "reply"
   },
   "214" : {
      "name" : "rpl_statsnline",
      "type" : "reply"
   },
   "215" : {
      "name" : "rpl_statsiline",
      "type" : "reply"
   },
   "216" : {
      "name" : "rpl_statskline",
      "type" : "reply"
   },
   "218" : {
      "name" : "rpl_statsyline",
      "type" : "reply"
   },
   "219" : {
      "name" : "rpl_endofstats",
      "type" : "reply"
   },
   "221" : {
      "name" : "rpl_umodeis",
      "type" : "reply"
   },
   "241" : {
      "name" : "rpl_statslline",
      "type" : "reply"
   },
   "242" : {
      "name" : "rpl_statsuptime",
      "type" : "reply"
   },
   "243" : {
      "name" : "rpl_statsoline",
      "type" : "reply"
   },
   "244" : {
      "name" : "rpl_statshline",
      "type" : "reply"
   },
   "251" : {
      "name" : "rpl_luserclient",
      "type" : "reply"
   },
   "252" : {
      "name" : "rpl_luserop",
      "type" : "reply"
   },
   "253" : {
      "name" : "rpl_luserunknown",
      "type" : "reply"
   },
   "254" : {
      "name" : "rpl_luserchannels",
      "type" : "reply"
   },
   "255" : {
      "name" : "rpl_luserme",
      "type" : "reply"
   },
   "256" : {
      "name" : "rpl_adminme",
      "type" : "reply"
   },
   "257" : {
      "name" : "rpl_adminloc1",
      "type" : "reply"
   },
   "258" : {
      "name" : "rpl_adminloc2",
      "type" : "reply"
   },
   "259" : {
      "name" : "rpl_adminemail",
      "type" : "reply"
   },
   "261" : {
      "name" : "rpl_tracelog",
      "type" : "reply"
   },
   "300" : {
      "name" : "rpl_none",
      "type" : "reply"
   },
   "301" : {
      "name" : "rpl_away",
      "type" : "reply"
   },
   "302" : {
      "name" : "rpl_userhost",
      "type" : "reply"
   },
   "303" : {
      "name" : "rpl_ison",
      "type" : "reply"
   },
   "305" : {
      "name" : "rpl_unaway",
      "type" : "reply"
   },
   "306" : {
      "name" : "rpl_nowaway",
      "type" : "reply"
   },
   "311" : {
      "name" : "rpl_whoisuser",
      "type" : "reply"
   },
   "312" : {
      "name" : "rpl_whoisserver",
      "type" : "reply"
   },
   "313" : {
      "name" : "rpl_whoisoperator",
      "type" : "reply"
   },
   "314" : {
      "name" : "rpl_whowasuser",
      "type" : "reply"
   },
   "315" : {
      "name" : "rpl_endofwho",
      "type" : "reply"
   },
   "317" : {
      "name" : "rpl_whoisidle",
      "type" : "reply"
   },
   "318" : {
      "name" : "rpl_endofwhois",
      "type" : "reply"
   },
   "319" : {
      "name" : "rpl_whoischannels",
      "type" : "reply"
   },
   "320" : { // freenode
      "name" : "rpl_whoisidentified",
      "type" : "reply"
   },
   "321" : {
      "name" : "rpl_liststart",
      "type" : "reply"
   },
   "322" : {
      "name" : "rpl_list",
      "type" : "reply"
   },
   "323" : {
      "name" : "rpl_listend",
      "type" : "reply"
   },
   "324" : {
      "name" : "rpl_channelmodeis",
      "type" : "reply"
   },
   "330" : { // ircu
      "name" : "rpl_whoisaccount",
      "type" : "reply"
   },
   "331" : {
      "name" : "rpl_notopic",
      "type" : "reply"
   },
   "332" : {
      "name" : "rpl_topic",
      "type" : "reply"
   },
   "338" : { // hybrid/ratbox/others
      "name" : "rpl_whoisactually",
      "type" : "reply"
   },
   "341" : {
      "name" : "rpl_inviting",
      "type" : "reply"
   },
   "342" : {
      "name" : "rpl_summoning",
      "type" : "reply"
   },
   "351" : {
      "name" : "rpl_version",
      "type" : "reply"
   },
   "352" : {
      "name" : "rpl_whoreply",
      "type" : "reply"
   },
   "353" : {
      "name" : "rpl_namreply",
      "type" : "reply"
   },
   "364" : {
      "name" : "rpl_links",
      "type" : "reply"
   },
   "365" : {
      "name" : "rpl_endoflinks",
      "type" : "reply"
   },
   "366" : {
      "name" : "rpl_endofnames",
      "type" : "reply"
   },
   "367" : {
      "name" : "rpl_banlist",
      "type" : "reply"
   },
   "368" : {
      "name" : "rpl_endofbanlist",
      "type" : "reply"
   },
   "369" : {
      "name" : "rpl_endofwhowas",
      "type" : "reply"
   },
   "371" : {
      "name" : "rpl_info",
      "type" : "reply"
   },
   "372" : {
      "name" : "rpl_motd",
      "type" : "reply"
   },
   "374" : {
      "name" : "rpl_endofinfo",
      "type" : "reply"
   },
   "375" : {
      "name" : "rpl_motdstart",
      "type" : "reply"
   },
   "376" : {
      "name" : "rpl_endofmotd",
      "type" : "reply"
   },
   "381" : {
      "name" : "rpl_youreoper",
      "type" : "reply"
   },
   "382" : {
      "name" : "rpl_rehashing",
      "type" : "reply"
   },
   "391" : {
      "name" : "rpl_time",
      "type" : "reply"
   },
   "392" : {
      "name" : "rpl_usersstart",
      "type" : "reply"
   },
   "393" : {
      "name" : "rpl_users",
      "type" : "reply"
   },
   "394" : {
      "name" : "rpl_endofusers",
      "type" : "reply"
   },
   "395" : {
      "name" : "rpl_nousers",
      "type" : "reply"
   },
   "401" : {
      "name" : "err_nosuchnick",
      "type" : "error"
   },
   "402" : {
      "name" : "err_nosuchserver",
      "type" : "error"
   },
   "403" : {
      "name" : "err_nosuchchannel",
      "type" : "error"
   },
   "404" : {
      "name" : "err_cannotsendtochan",
      "type" : "error"
   },
   "405" : {
      "name" : "err_toomanychannels",
      "type" : "error"
   },
   "406" : {
      "name" : "err_wasnosuchnick",
      "type" : "error"
   },
   "407" : {
      "name" : "err_toomanytargets",
      "type" : "error"
   },
   "409" : {
      "name" : "err_noorigin",
      "type" : "error"
   },
   "411" : {
      "name" : "err_norecipient",
      "type" : "error"
   },
   "412" : {
      "name" : "err_notexttosend",
      "type" : "error"
   },
   "413" : {
      "name" : "err_notoplevel",
      "type" : "error"
   },
   "414" : {
      "name" : "err_wildtoplevel",
      "type" : "error"
   },
   "421" : {
      "name" : "err_unknowncommand",
      "type" : "error"
   },
   "422" : {
      "name" : "err_nomotd",
      "type" : "error"
   },
   "423" : {
      "name" : "err_noadmininfo",
      "type" : "error"
   },
   "424" : {
      "name" : "err_fileerror",
      "type" : "error"
   },
   "431" : {
      "name" : "err_nonicknamegiven",
      "type" : "error"
   },
   "432" : {
      "name" : "err_erroneusnickname",
      "type" : "error"
   },
   "433" : {
      "name" : "err_nicknameinuse",
      "type" : "error"
   },
   "436" : {
      "name" : "err_nickcollision",
      "type" : "error"
   },
   "441" : {
      "name" : "err_usernotinchannel",
      "type" : "error"
   },
   "442" : {
      "name" : "err_notonchannel",
      "type" : "error"
   },
   "443" : {
      "name" : "err_useronchannel",
      "type" : "error"
   },
   "444" : {
      "name" : "err_nologin",
      "type" : "error"
   },
   "445" : {
      "name" : "err_summondisabled",
      "type" : "error"
   },
   "446" : {
      "name" : "err_usersdisabled",
      "type" : "error"
   },
   "451" : {
      "name" : "err_notregistered",
      "type" : "error"
   },
   "461" : {
      "name" : "err_needmoreparams",
      "type" : "error"
   },
   "462" : {
      "name" : "err_alreadyregistred",
      "type" : "error"
   },
   "463" : {
      "name" : "err_nopermforhost",
      "type" : "error"
   },
   "464" : {
      "name" : "err_passwdmismatch",
      "type" : "error"
   },
   "465" : {
      "name" : "err_yourebannedcreep",
      "type" : "error"
   },
   "467" : {
      "name" : "err_keyset",
      "type" : "error"
   },
   "471" : {
      "name" : "err_channelisfull",
      "type" : "error"
   },
   "472" : {
      "name" : "err_unknownmode",
      "type" : "error"
   },
   "473" : {
      "name" : "err_inviteonlychan",
      "type" : "error"
   },
   "474" : {
      "name" : "err_bannedfromchan",
      "type" : "error"
   },
   "475" : {
      "name" : "err_badchannelkey",
      "type" : "error"
   },
   "481" : {
      "name" : "err_noprivileges",
      "type" : "error"
   },
   "482" : {
      "name" : "err_chanoprivsneeded",
      "type" : "error"
   },
   "483" : {
      "name" : "err_cantkillserver",
      "type" : "error"
   },
   "491" : {
      "name" : "err_nooperhost",
      "type" : "error"
   },
   "501" : {
      "name" : "err_umodeunknownflag",
      "type" : "error"
   },
   "502" : {
      "name" : "err_usersdontmatch",
      "type" : "error"
   },
   "671" : {
      "name" : "rpl_whoissecure",
      "type" : "reply"
   }
}; // }}}

function Client(server, nick, opt) {
  var self = this, whois = {};
  self.opt = {
    server: server,
    nick: nick,
    password: null,
    userName: 'nodebot',
    realName: 'nodeJS IRC client', 
    port: 6667,
    debug: false,
    showErrors: false,
    autoConnect: true,
    autoRejoin: true,
    channels: [],
    retryCount: null,
    retryDelay: 2000,
    secure: false,
  };

  if (typeof arguments[2] == 'object') {
    var keys = Object.keys(self.opt);
    for (var i = 0; i < keys.length; i++) {
      var k = keys[i];
      if (arguments[2][k] !== undefined)
        self.opt[k] = arguments[2][k];
    }
  }

  // TODO - fail if nick or server missing
  // TODO - fail if username has a space in it
  if (self.opt.autoConnect)
    self.connect();

  self.on("raw", function (message) { // {{{
    switch (message.command) {
      case "001":
      case "002":
      case "003":
      case "004":
      case "005":
      case "250":
      case "265":
      case "266":
      case "rpl_luserclient":
      case "rpl_luserop":
      case "rpl_luserchannels":
      case "rpl_luserme":
      case "rpl_luserunknown":
        if (message.command === "004")
          self.emit('ready');
        // Welcome stuff, ignoring for now
        break;
      case "err_nicknameinuse":
        if (!self.prevnick) {
          if (typeof(self.opt.nickMod) == 'undefined')
            self.opt.nickMod = 0;
          self.opt.nickMod++;
          self.send("NICK", self.opt.nick + self.opt.nickMod);
          self.nick = self.opt.nick + self.opt.nickMod;
        } else
          self.nick = self.prevnick;
        break;
      case "rpl_whoisuser":
        var who = message.args[1];
        if (!whois[who])
          whois[who] = {};
        whois[who].username = message.args[2];
        whois[who].host = message.args[3];
        whois[who].realname = message.args.slice(5).join(' ');
      break;
      case "rpl_whoisidle":
        var who = message.args[1];
        whois[who].idle = parseInt(message.args[2]);
        try {
          whois[who].signon = parseInt(message.args[3]);
        } catch (err) {}
      break;
      case "rpl_whoisoperator":
        whois[message.args[1]].isServerOp = true;
      break;
      case "rpl_whoischannels":
        var who = message.args[1], chans = message.args.slice(2);
        if (!whois[who].channels)
          whois[who].channels = [];
        for (var i=0; i<chans.length; i++) {
          if (chans[i][0] !== '#' && chans[i][0] !== '&')
            whois[who].channels.push({ name: chans[i].substring(1), mode: chans[i][0] });
          else
            whois[who].channels.push({ name: chans[i] });
        }
      break;
      case "rpl_whoisserver":
        var who = message.args[1];
        whois[who].server = { host: message.args[2], info: message.args[3] };
      break;
      case "378":
        if (message.args[2] && message.args[2].substr(0, 3) === 'is ') {
          // hostinfo
          whois[message.args[1]].hostinfo = message.args[2];
        }
      break;
      /*case "rpl_whoisidentified":
        whois[message.args[1]].identifiedAs = message.args[2];
      break;*/
      /*case "rpl_whoisactually":
        var host = messages.args[2].split('@'), username;
        if (host.length > 1) {
          username = host[0];
          host = host.slice(1).join('@');
        } else
          host = host[0];
        whois[message.args[1]].actually = { username: username, host: host, ip: message.args[3] };
      break;*/
      case "rpl_whoissecure":
        whois[message.args[1]].secureConn = true;
      break;
      case "rpl_whoisaccount":
        whois[message.args[1]].account = message.args[2];
      break;
      case "rpl_endofwhois":
        var who = message.args[1];
        if (whois[who]) {
          self.emit('whois', who, whois[who]);
          delete whois[who];
        }
      break;
      case "PING":
        self.send("PONG", message.args[0]);
      break;
      case "QUIT":
        for (var i=0,keys=Object.keys(self.chans); i < keys.length; i++) {
          if (Object.keys(self.chans[keys[i]].users).indexOf(message.nick) > -1)
            self.emit('quit', keys[i], message.nick, message.args.join(" "));
        }

        if (self.opt.debug)
          util.log("QUIT: " + message.prefix + " " + message.args.join(" "));
      break;
      case "NOTICE":
        var from = message.nick, to = message.args[0], text = message.args[1];
        self.emit('notice', from, to, text);

        if (self.opt.debug && to == self.nick)
          util.log('NOTICE from ' + (from ? from : '(server)') + ': ' + text);
      break;
      case "MODE":
        if (self.opt.debug)
          util.log("MODE:" + message.args[0] + " sets mode: " + message.args[1]);
        if (message.args.length >= 3) {
          var channel = self.chans[message.args[0]],
              nicklist_offset = 2,
              mode = message.args[1].split(''),
              adding = (mode.shift() === "+");
          for (var i = 0; i < mode.length; i++) {
            self.emit('mode', message.args[0], message.nick, message.args[nicklist_offset+i], (adding ? '+' : '-') + mode[i]);
            if (mode[i] == 'o') {
              if (adding)
                channel.users[message.args[nicklist_offset+i]] = '@';
              else
                channel.users[message.args[nicklist_offset+i]] = '';
            } else if (mode[i] == 'v') {
              if (adding)
                channel.users[message.args[nicklist_offset+i]] = '+';
              else
                channel.users[message.args[nicklist_offset+i]] = '';
            }
          }
        }
      break;
      case "rpl_away":
        self.emit('away', message.args[1], message.args[2]);
      break;
      case "rpl_nowaway":
        self.away = true;
      break;
      case "rpl_unaway":
        self.away = false;
      break;
      case "NICK":
        var oldnick = message.nick, newnick = message.args[0];
        for (var i=0,keys=Object.keys(self.chans); i<keys.length; i++) {
          if (Object.keys(self.chans[keys[i]].users).indexOf(oldnick) > -1) {
            self.chans[keys[i]].users[newnick] = self.chans[keys[i]].users[oldnick];
            delete self.chans[keys[i]].users[oldnick];
            self.emit('nick', keys[i], oldnick, newnick);
          }
        }
        self.emit('nick', undefined, oldnick, newnick);
      break;
      case "rpl_motdstart":
        self.motd = message.args[1] + "\n";
      break;
      case "rpl_motd":
        self.motd += message.args[1] + "\n";
      break;
      case "rpl_endofmotd":
      case "err_nomotd":
        self.motd += message.args[1] + "\n";
        self.emit('motd', self.motd);
      break;
      case "rpl_namreply":
        var channel = self.chans[message.args[2]];
        var users = message.args[3].split(/ +/);
        users.forEach(function (user) {
          var match = user.match(/^([@+])?(.*)$/);
          if (!match[1])
            match[1] = "";
          channel.users[match[2]] = match[1];
        });
      break;
      case "rpl_endofnames":
        var channel = self.chans[message.args[1]];
        self.emit('names', message.args[1], channel.users);
        self.send('MODE', message.args[1]);
      break;
      case "rpl_topic":
        var channel = self.chans[message.args[1]];
        if (channel)
          channel.topic = message.args[2];
      break;
      case "333":
        var channel = self.chans[message.args[1]];
        if (channel) {
          channel.topicBy = message.args[2];
          // channel, topic, nick
          self.emit('topic', message.args[1], channel.topic, channel.topicBy, true);
        }
      break;
      case "TOPIC":
        // channel, topic, nick
        self.emit('topic', message.args[0], message.args[1], message.nick);

        var channel = self.chans[message.args[0]];
        if (channel) {
          channel.topic = message.args[1];
          channel.topicBy = message.nick;
        }
      break;
      case "rpl_channelmodeis":
        var channel = self.chans[message.args[1]];
        if (channel)
          channel.mode = message.args[2];
      break;
      case "329":
        var channel = self.chans[message.args[1]];
        if (channel)
          channel.created = message.args[2];
      break;
      case "JOIN":
        // channel, who
        self.emit('join', message.args[0], message.nick);
        self.emit('join' + message.args[0], message.nick);
        if (self.nick == message.nick) {
          self.chans[message.args[0]] = {
            users: {},
          };
        }
        else {
          var channel = self.chans[message.args[0]];
          channel.users[message.nick] = '';
        }
      break;
      case "PART":
        // channel, who, reason
        self.emit('part', message.args[0], message.nick, message.args[1]);
        self.emit('part' + message.args[0], message.nick, message.args[1]);
        if (self.nick == message.nick)
          delete self.chans[message.args[0]];
        else {
          var channel = self.chans[message.args[0]];
          delete channel.users[message.nick];
        }
      break;
      case "KICK":
        // channel, who, by, reason
        self.emit('kick', message.args[0], message.args[1], message.nick, message.args[2]);
        self.emit('kick' + message.args[0], message.args[1], message.nick, message.args[2]);

        if (self.nick == message.args[1])
          delete self.chans[message.args[0]];
        else {
          var channel = self.chans[message.args[0]];
          delete channel.users[message.args[1]];
        }
      break;
      case "KILL":
        var nick = message.args[0];
        for (var channel in self.chans)
          delete self.chans[channel].users[nick];
      break;
      case "PRIVMSG":
        var from = message.nick;
        var to   = message.args[0];
        var text = message.args[1];
        self.emit('message', from, to, text);
        if (to.match(/^[&#+!]/))
          self.emit('message' + to, from, text);
        if (to == self.nick)
          self.emit('pm', from, text);

        if (self.opt.debug && to == self.nick)
          util.log('GOT MESSAGE from ' + from + ': ' + text);
      break;
      default:
        if (message.commandType == 'error') {
          self.emit('error', message);
          if (self.opt.showErrors)
            util.log("\033[01;31mERROR: " + util.inspect(message) + "\033[0m");
        } else {
          self.emit('unhandledMessage', message);
          if (self.opt.debug)
            util.log("\033[01;31mUnhandled message: " + util.inspect(message) + "\033[0m");
        }
    }
  }); // }}}

  self.on('kick', function(channel, who, by, reason) {
    if (self.opt.autoRejoin)
      self.send('JOIN', channel);
  });
  self.on('motd', function (motd) {
    self.opt.channels.forEach(function(channel) {
      self.send('JOIN', channel);
    });
  });

  process.EventEmitter.call(this);
}

util.inherits(Client, process.EventEmitter);

Client.prototype.conn = null;
Client.prototype.chans = {};
Client.prototype.connect = function (retryCount) { // {{{
  retryCount = retryCount || 0;
  var self = this;
  self.chans = {};
  self.prevnick = undefined;
  self.conn = net.createConnection(self.opt.port, self.opt.server);
  self.conn.requestedDisconnect = false;
  if (self.opt.secure) {
    // Set up a secure (SSL) connection
    if (typeof self.opt.secure !== 'object') {
      // Do not verify connection
      self.opt.secure = require('crypto').createCredentials({});
    } else {
      // Verify connection
      self.conn.on("secure", function(){
        if(!self.conn.verifyPeer()) {
          if (self.opt.debug) {
            util.log('Warning: failed to verify SSL peer certificate'+
                    ' -- aborting connection');
          }
          this.end();
        }
      });
    }
    self.conn.setSecure(self.opt.secure);
  }
  self.conn.setTimeout(0);
  self.conn.setEncoding('utf8');
  self.conn.on("connect", function () {
    if (self.opt.password !==  null)
      self.send("PASS", self.opt.password);
    self.send("NICK", self.opt.nick);
    self.nick = self.opt.nick;
    self.send("USER", self.opt.userName, 8, "*", self.opt.realName);
    self.emit("connect");
  });
  var buffer = '';
  self.conn.on("data", function (chunk) {
    buffer += chunk;
    var lines = buffer.split("\r\n");
    buffer = lines.pop();
    lines.forEach(function (line) {
      var message = parseMessage(line);
      self.emit('raw', message);
    });
  });
  self.conn.on("end", function() {
    if (self.conn.requestedDisconnect)
      return;
    if (self.opt.debug)
      util.log('Disconnected: reconnecting');
    if (self.opt.retryCount !== null && retryCount >= self.opt.retryCount) {
      if (self.opt.debug)
        util.log('Maximum retry count (' + self.opt.retryCount + ') reached. Aborting');
      self.emit('abort', self.opt.retryCount);
      return;
    }

    if (self.opt.debug)
      util.log('Waiting ' + self.opt.retryDelay + 'ms before retrying');
    setTimeout(function() {
      self.connect(retryCount + 1);
    }, self.opt.retryDelay);
  });
}; // }}}
Client.prototype.disconnect = function (message) { // {{{
  message = message || "Goodbye";
  var self = this;
  if (self.conn.readyState == 'open')
    self.send("QUIT", message);
  self.conn.requestedDisconnect = true;
  self.conn.end();    
}; // }}}
Client.prototype.send = function(command) { // {{{
  var args = Array.prototype.slice.call(arguments);
  if (args.length > 1) {
    if (args[args.length-1] !== false)
      args[args.length-1] = ":" + args[args.length-1];
    else
      args.splice(args.length-1, 1);
  }

  // Remove the command
  args.shift();

  if (this.opt.debug)
    util.log('SEND: ' + command + " " + args.join(" "));

  this.conn.write(command + (args.length ? " " + args.join(" ") : "") + "\r\n");
}; // }}}
Client.prototype.join = function(channel, callback) { // {{{
  if (typeof(callback) == 'function') {
    var callbackWrapper = function () {
      this.removeListener('join' + channel, callbackWrapper);
      return callback.apply(this, arguments);
    };
    this.on('join' + channel, callbackWrapper);
  }

  this.send('JOIN', channel);
} // }}}
Client.prototype.part = function(channel, reason, callback) { // {{{
  if (typeof(reason) == 'function') {
    callback = reason;
    reason = undefined;
  }
  if (typeof(callback) == 'function') {
    var callbackWrapper = function () {
      this.removeListener('part' + channel, callbackWrapper);
      return callback.apply(this, arguments);
    };
    this.on('part' + channel, callbackWrapper);
  }

  if (reason)
    this.send('PART', channel, ''+reason);
  else
    this.send('PART', channel);
} // }}}
Client.prototype.say = function(target, text) { // {{{
  this.send('PRIVMSG', target, text);
} // }}}

/*
 * parseMessage(line)
 * 
 * takes a raw "line" from the IRC server and turns it into an object with
 * useful keys
 */
function parseMessage(line) { // {{{
  var message = {};
  var match;

  // Parse prefix
  if (match = line.match(/^:([^ ]+) +/)) {
    message.prefix = match[1];
    line = line.replace(/^:[^ ]+ +/, '');
    if (match = message.prefix.match(/^([_a-zA-Z0-9\[\]\\`^{}|-]*)(!([^@]+)@(.*))?$/)) {
      message.nick = match[1];
      message.user = match[3];
      message.host = match[4];
    }
    else
      message.server = message.prefix;
  }

  // Parse command
  match = line.match(/^([^ ]+) +/);
  message.command = match[1];
  message.rawCommand = match[1];
  message.commandType = 'normal';
  line = line.replace(/^[^ ]+ +/, '');

  if (replyFor[message.rawCommand]) {
    message.command     = replyFor[message.rawCommand].name;
    message.commandType = replyFor[message.rawCommand].type;
  }

  message.args = [];
  var middle, trailing;

  // Parse parameters
  if (line.indexOf(':') != -1) {
    var index = line.indexOf(':');
    middle = line.substr(0, index).replace(/ +$/, "");
    trailing = line.substr(index+1);
  } else
    middle = line;

  if (middle.length)
    message.args = middle.split(/ +/);

  if (typeof(trailing) != 'undefined' && trailing.length)
    message.args.push(trailing);

  return message;
} // }}}
