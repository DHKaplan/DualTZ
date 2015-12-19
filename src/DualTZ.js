var myAPIKey = "ebe0a78125281118a038b2a62aab07c8";

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  console.log("addEventListener: showConfiguratonPage\n");
  Pebble.openURL('http://www.wticalumni.com/DHK/DualTZ-V1.00.htm');
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var dict = JSON.parse(decodeURIComponent(e.response));

    //Send a string to Pebble
    Pebble.sendAppMessage(dict,
      function(e) {
        console.log("   Send successful.");
      },
      function(e) {
        console.log("   Send failed!");
      });
  });

//********************************************************************
function fetchWeather(latitude, longitude) {
  var req = new XMLHttpRequest();

  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1" + "&APPID=" + myAPIKey, false); //was true

  req.onreadystatechange = function(e) { // was req.onload
    if (req.readyState == 4) {
      if (req.status == 200) {
        console.log("req.status=200, Response Text Follows: ");
        console.log(req.responseText);

        var response = JSON.parse(req.responseText);

        // Convert to Centigrade
        var temperature = Math.round(response.main.temp - 273.15);

        var ftemp = (temperature * (9 / 5)) + 32;
        console.log("Java Temp in F: " + ftemp);

        var city = response.name;
        console.log("City: " + response.name);

        Pebble.sendAppMessage({
          "WEATHER_TEMPERATURE_KEY": temperature + "\u00B0C",
          "WEATHER_CITY_KEY": city
        });

      } else {
        console.log("Error in req.status: " + req.status);
        Pebble.sendAppMessage({
          "WEATHER_TEMPERATURE_KEY": "N/A",
          "WEATHER_CITY_KEY": "N/A"
        });

      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
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
  console.log("addEventListener ready message: " + e.ready);
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  console.log("locationWatcher e.type: " + e.type);
});

Pebble.addEventListener("appmessage", function(e) {
console.log("Appmessage Received in JS"); 
});