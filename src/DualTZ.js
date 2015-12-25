var myAPIKey = "ebe0a78125281118a038b2a62aab07c8";

Pebble.addEventListener('showConfiguration', function(e) {
    // Show config page
    console.log("addEventListener: showConfigurationPage\n");
    Pebble.openURL('http://www.wticalumni.com/DHK/DualTZ-V1.00.htm');
});

Pebble.addEventListener('webviewclosed',

    function(e) {
        var dict = JSON.parse(decodeURIComponent(e.response));

      //Send a string to Pebble
        Pebble.sendAppMessage(dict,

            function(e) {
                console.log("   webviewclosed Send successful.");
            },

            function(e) {
                console.log("   webviewclosed Send failed!");
            });
    });

//********************************************************************
var xhrRequest = function(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    // Construct URL
    var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
        pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey;
    console.log("Lat = " + pos.coords.latitude + " Long = " + pos.coords.longitude);
    
    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET',
        function(responseText) {
            // responseText contains a JSON object with weather info
            var json = JSON.parse(responseText);
            console.log("Response Text: " + responseText);

            // Temperature in Kelvin requires adjustment
            var tempC = Math.round(json.main.temp - 273.15);
            console.log("Temperature C is " + tempC);

            // City
            var city = json.name;
            console.log("City is " + city);

            // Assemble dictionary using our keys
            var dictionary = {
                "WEATHER_TEMPERATURE_KEY": tempC + "\u00B0C",
                "WEATHER_CITY_KEY": city
            };

            // Send to Pebble
            Pebble.sendAppMessage(dictionary,
                function(e) {
                    console.log("Weather info sent to Pebble successfully!");
                },
                function(e) {
                    console.log("Error sending weather info to Pebble!");
                }
            );
        }
    );
}

function locationError(err) {
    console.warn('location error (' + err.code + '): ' + err.message);
    Pebble.sendAppMessage({
        "WEATHER_CITY_KEY": "Loc Unavailable",
        "WEATHER_TEMPERATURE_KEY": "N/A"
    });
}

var locationOptions = {
    "timeout": 15000,
    "maximumAge": 60000
};

Pebble.addEventListener("ready", function(e) {
    console.log("Ready msg rcvd: Phone-Watch Communication Established, GPS find initiated");
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);

});

Pebble.addEventListener("appmessage", function(e) {
    console.log("Appmessage msg rcvd: Phone-Watch Communication Passsed, GPS find initiated");
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
});