var app = {
    defaults: {
        mqtt_clnt: null,
        ip: "127.0.0.1",
        port: 3000,
        g: null,
        gageValue: 0.00
    },
    init: function() {
        $('[type="checkbox"]').bootstrapSwitch(); // init
        app.bindEvents();
        app.initGage();
        app.initMqtt();
    },
    initGage: function() {
        app.defaults.g = new JustGage({
            id: "gauge",
            value: app.defaults.gageValue,
            min: 0.00,
            max: 10.00,
            decimals: true,
            title: "Voltage"
        });
    },
    initMqtt: function() {
        app.defaults.mqtt_clnt = new Paho.MQTT.Client(app.defaults.ip, app.defaults.port, "myclientid_" + parseInt(Math.random() * 100, 10));
        
        app.defaults.mqtt_clnt.onConnectionLost = function (responseObject) {
            console.log("connection lost: " + responseObject.errorMessage);
        };

        app.defaults.mqtt_clnt.onMessageArrived = function (message) {
            console.log(message.destinationName, ' -- ', message.payloadString);
            // update gauge
            app.defaults.gageValue = parseFloat(message.payloadString);
            app.defaults.g.refresh(app.defaults.gageValue);
        };

        var options = {
            timeout: 3,
            onSuccess: function () {
                console.log("mqtt connected");
                app.defaults.mqtt_clnt.subscribe('/sensors/rotary/3', {qos: 1});      
            },
            onFailure: function (message) {
                console.log("Connection failed: " + message.errorMessage);
            }
        };
        app.defaults.mqtt_clnt.connect(options);
    },
    bindEvents: function() {
        $('#do0').on('switchChange.bootstrapSwitch', function(event, state) {
            app.setStatus(16, state);
        });
        $('#do1').on('switchChange.bootstrapSwitch', function(event, state) {
            app.setStatus(17, state);
        });
    },
    setStatus: function(id, state) {
        var message = new Paho.MQTT.Message(state ? "1" : "0");
        message.destinationName = "/sensors/led/" + id;
        app.defaults.mqtt_clnt.send(message);
    }
};


$(document).ready(function(){
    app.init();
});