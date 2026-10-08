// PebbleKit JS - Prototyp (Schritt 1): schickt ein fest eingebautes Test-PNG
// in Stuecken per AppMessage an die Uhr.

var TEST_PANEL_B64 = require('./test_panel');

// Pro Tupel kostet ein AppMessage-Dictionary 7 Byte Header, dazu 1 Byte fuer
// das Dictionary selbst. Wir senden OFFSET + TOTAL_SIZE (je int32) + DATA.
var DICT_OVERHEAD = 1 + (7 + 4) + (7 + 4) + 7;
var MAX_RETRIES = 3;

var B64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

// Eigener Decoder, weil atob() nicht in jeder PebbleKit-JS-Umgebung existiert.
function base64ToBytes(str) {
  var lookup = {};
  for (var i = 0; i < B64.length; i++) lookup[B64.charAt(i)] = i;
  str = str.replace(/[^A-Za-z0-9+\/]/g, '');
  var bytes = [];
  for (var j = 0; j < str.length; j += 4) {
    var a = lookup[str.charAt(j)], b = lookup[str.charAt(j + 1)];
    var c = lookup[str.charAt(j + 2)], d = lookup[str.charAt(j + 3)];
    bytes.push((a << 2) | (b >> 4));
    if (c !== undefined) bytes.push(((b & 15) << 4) | (c >> 2));
    if (d !== undefined) bytes.push(((c & 3) << 6) | d);
  }
  return bytes;
}

function sendError(text) {
  Pebble.sendAppMessage({ ERROR: text });
}

function sendPng(bytes, inboxSize) {
  var chunkSize = Math.max(64, inboxSize - DICT_OVERHEAD);
  var offset = 0;
  var retries = 0;
  var started = Date.now();

  function sendNext() {
    if (offset >= bytes.length) {
      console.log('Panel gesendet: ' + bytes.length + ' Bytes in ' +
                  (Date.now() - started) + ' ms (Chunk ' + chunkSize + ' Bytes)');
      return;
    }
    var chunk = bytes.slice(offset, offset + chunkSize);
    Pebble.sendAppMessage({ TOTAL_SIZE: bytes.length, OFFSET: offset, DATA: chunk },
      function () {
        offset += chunk.length;
        retries = 0;
        sendNext();
      },
      function (e) {
        if (++retries > MAX_RETRIES) {
          console.log('Senden abgebrochen bei Offset ' + offset);
          sendError('Uebertragung fehlgeschlagen');
          return;
        }
        console.log('NACK bei Offset ' + offset + ', neuer Versuch ' + retries);
        setTimeout(sendNext, 100 * retries);
      });
  }
  sendNext();
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS bereit');
  Pebble.sendAppMessage({ READY: 1 });
});

Pebble.addEventListener('appmessage', function (e) {
  var msg = e.payload;
  if (msg.REQUEST === undefined) return;
  var inboxSize = msg.INBOX_SIZE || 512;
  console.log('Panel ' + msg.REQUEST + ' angefordert, Inbox ' + inboxSize + ' Bytes');
  sendPng(base64ToBytes(TEST_PANEL_B64), inboxSize);
});
