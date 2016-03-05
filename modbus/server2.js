var net = require('net');
var modbus = require('modbus-tcp');

var server = net.createServer(function (socket) {

	var modbusServer = new modbus.Server();
	modbusServer.writer().pipe(socket);
	socket.pipe(modbusServer.reader());

	var register = [1, 1, 1, 1];

	modbusServer.on("read-holding-registers", function (from, to, reply) {
		console.log('reading:', from, to);

		var buffer = new Buffer(2);
		buffer.writeUInt16BE(65535, 0);
		return reply(null, [buffer]);
	});

	modbusServer.on("read-coils", function (from, to, reply) {
		console.log('reading:', from, to);

	    return reply(null, register);
	});

	modbusServer.on("write-single-coil", function (from, to, reply) {
	    console.log('writing:', from, to);
	    if (from == 16) { register[0] = to.readUInt16BE(0);}
    	if (from == 17) { register[1] = to.readUInt16BE(0);}

	    return reply(null, register);
	});
});

server.listen(502, function() {
	console.log('listening...');
});
