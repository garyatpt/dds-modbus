var modbus  = require('jsmodbus'),
    util    = require('util');

modbus.setLogger(function (msg) { util.log(msg); });

var register = [true, true];

var readCoils = function (start, quant) {
    return [register];
};

var writeSingleCoil = function (adr, value) {

    console.log('write single coil (' + adr + ', ' + value + ')');
    if (adr == 16) { register[0] = value;}
    if (adr == 17) { register[1] = value;}
    return [];

};

var writeSingleRegister = function (adr, value) {

    console.log('write single register (' + adr + ', ' + value + ')');

    return [];  

};

modbus.createTCPServer(502, '127.0.0.1', function (err, server) {

    if (err) {
        console.log(err);
        return;
    }

    server.addHandler(1, readCoils);
    server.addHandler(5, writeSingleCoil);
    //server.addHandler(6, writeSingleRegister);

});