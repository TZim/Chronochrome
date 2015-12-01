var temperature = 999;
var localtimezone = 99;
var KEY_STOCKS_NA = 10000;
var djia = KEY_STOCKS_NA;
var nasdaq = KEY_STOCKS_NA;
var sunrise = 0;
var sunset = 0;
var OWMID = "&appid=bf84ccfefb4efc97324fdb60fdd42038";

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

//xx function locationSuccess(lat, long) { //xx
function locationSuccess(pos) {
  // console.log("position: " + pos);
  // console.log(" latitude: " + pos.coords.latitude);

  // Construct URL
  var wx_url;
  if (false) { // true == FAKE_LOC
      wx_url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
                    42.34 + "&lon=" + -71.1 + OWMID;
  } else {
      wx_url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
                    pos.coords.latitude + "&lon=" + pos.coords.longitude + OWMID;
  }

  // Send request to OpenWeatherMap
  xhrRequest(wx_url, 'GET', 
    function(responseText) {
      // console.log(responseText);
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      temperature = Math.round(json.main.temp - 273.15);
      // console.log("Temperature is " + temperature);
      
      sunrise = json.sys.sunrise;
      sunset = json.sys.sunset;

      // Conditions
      //var conditions = json.weather[0].main;      
      //console.log("Conditions are " + conditions);
    }      
  );
}

function locationError(err) {
   console.log('location error (' + err.code + '): ' + err.message);
}

function getWeather() {
  //xx locationSuccess(42.34, -71.1);  //xx
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function refreshData() {
  getData();
  setTimeout(function(){sendData();}, 1000 * 10);
}

function getData() {
    
  // Get local timezone
  localtimezone = (new Date()).getTimezoneOffset()/60;
  // console.log("UTC offset is " + localtimezone);
  
  getWeather();
  
  // Get stock-market data
  djia = 0;
  nasdaq = 0;
  var stocks_url = "http://finance.google.com/finance/";
  // Send requests to Google Finance:
  var start, end;
  var textnum;
  var base, delta;
  djia = KEY_STOCKS_NA;
  nasdaq = KEY_STOCKS_NA;
  
  xhrRequest(stocks_url, 'GET', 
    function(responseText) {
      responseText = responseText.substring(3);
      start = responseText.indexOf("/finance?q=INDEXDJX:.DJI");
      start = responseText.indexOf("Dow Jones", start);
      start = responseText.indexOf("<span", start);
      start = responseText.indexOf(">", start) + 1;
      end = responseText.indexOf("</span", start);
      textnum = responseText.substring(start, end);
      textnum = textnum.replace(",", "");
      base = parseFloat(textnum);
      start = responseText.indexOf("<span", end);
      start = responseText.indexOf(">", start) + 1;
      end = responseText.indexOf("</span", start);
      textnum = responseText.substring(start, end);
      textnum = textnum.replace(",", "");
      textnum = textnum.replace("+", "");
      delta = parseFloat(textnum);
      base = KEY_STOCKS_NA * delta / (base - delta);
      if (!isNaN(base) && isFinite(base))
        djia = base;
    }
  );

  xhrRequest(stocks_url, 'GET', 
    function(responseText) {
      responseText = responseText.substring(3);
      start = responseText.indexOf("/finance?q=INDEXNASDAQ:.IXIC");
      start = responseText.indexOf("Nasdaq", start);
      start = responseText.indexOf("<span", start);
      start = responseText.indexOf(">", start) + 1;
      end = responseText.indexOf("</span", start);
      textnum = responseText.substring(start, end);
      textnum = textnum.replace(",", "");
      base = parseFloat(textnum);
      start = responseText.indexOf("<span", end);
      start = responseText.indexOf(">", start) + 1;
      end = responseText.indexOf("</span", start);
      textnum = responseText.substring(start, end);
      textnum = textnum.replace(",", "");
      textnum = textnum.replace("+", "");
      delta = parseFloat(textnum);
      base = KEY_STOCKS_NA * delta / (base - delta);
      if (!isNaN(base) && isFinite(base))
        nasdaq = base;
    }
  );

}
  
function sendData() {
  // Assemble dictionary using our keys
  var dictionary = {
    "KEY_TEMPERATURE": temperature,
    "KEY_UTC_OFFSET": localtimezone,
    "KEY_DJIA": djia,
    "KEY_NASDAQ": nasdaq,
    "KEY_SUNRISE" : sunrise,
    "KEY_SUNSET" : sunset
  };
      
  // Send to Pebble
  Pebble.sendAppMessage(dictionary,
    function(e) {
      // console.log("Info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending info to Pebble!");
    }
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    // console.log("PebbleKit JS ready!");

    // Send the initial data
    refreshData();
  }
);

// Listen for when an AppMessage ping is received
Pebble.addEventListener('appmessage',
  function(e) {
    // console.log("AppMessage received!");
    // Reply to ping by sending data.
    refreshData();
  }                     
);