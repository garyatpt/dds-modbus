var modbus = require('jsmodbus');

// create a modbus client
var client = modbus.createTCPClient(502, '127.0.0.1', function (err) {
    if (err) {
        console.log(err);
        process.exit(0);
    }
});

//FC3
client.readHoldingRegister(3, 1, function (resp, err) {
    console.log(resp); 
});

//FC1
client.readCoils(16, 2, function (resp, err) {
    console.log(resp.coils.slice(0, 2));
});

//FC5
client.writeSingleCoil(16, false, function (resp, err) {
    console.log(resp);
});

//FC5
client.writeSingleCoil(17, false, function (resp, err) {
    console.log(resp);
});

//FC1
client.readCoils(16, 2, function (resp, err) {
    console.log(resp.coils.slice(0, 2));
});

setTimeout(function(){
	client.close();
}, 1000);