var app = require('http').createServer(requestHandler),
io = require('socket.io').listen(app),
path = require('path'),
fs = require('fs'),
url = require("url");

app.listen(3000);

function requestHandler(req, res) {
    var uri = url.parse(req.url).pathname;
    var filename = path.join(process.cwd(), uri);

    if (fs.statSync(filename).isDirectory())
	filename += '/civitas.html';

    fs.readFile(filename, "binary", function(err, file) {
	if (err) {
	    console.log(err);
	    res.writeHead(500);
	    return res.end('<h1>Sorry, the page you are looking for cannot be found.</h1>');
	}
	res.writeHead(200);
	res.write(file, "binary");
	res.end();
    });
};

var sys = require('sys'),
net = require('net'),
mqtt = require('mqtt'),

mysocket = require('socket.io').listen(5000),
client = mqtt.createClient(1883, 'civitas');

client.options.reconnectPeriod = 0;

client.on('message', function(topic, message) {
  console.log(message);
  sys.puts(topic+'='+message);
  mysocket.sockets.in(topic).emit('mqtt',{'topic': String(topic), 'payload':String(message)});
});

mysocket.sockets.on('connection', function (sock) {
  sock.on('subscribe', function (data) {
    console.log('Subscribing to '+data.topic);
    sock.join(data.topic);
    client.subscribe(data.topic);
  });
});
