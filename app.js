var celsius = 0;
var current_temp = 0;
var standby = 0;
var api_temp = "";
var server_down = 0;
var weatherApiCalled = 0;



Pebble.addEventListener("appmessage",
 function(e) {
   if (e.payload){
     if (e.payload.celsius == 'C') {
        celsius = 0;
     }else if(e.payload.celsius == 'F'){
       celsius = 1; 
     }
     if (e.payload.standby == 'N'){
       standby = 0;
     } else if (e.payload.standby == 'S'){
       standby = 1;
     }
     if (e.payload.api == 'A'){
       current_temp = 0;
     } else if (e.payload.api == 'T'){
       current_temp = 1;
       getWeather();
     }
   }
   
 if (weatherApiCalled == 0){
   weatherApiCalled = 1;
   getWeather();
 }


 sendToServer();
 }
); 
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
      callback(this.responseText);
    };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // We will request the weather here
  var weatherUrl = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude +'&units=metric&appid=<Your Api Key>';
  xhrRequest(weatherUrl, 'GET', function(responseText){
    var respond = JSON.parse(responseText);
    api_temp = respond.main.temp;
    
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}




function sendToServer() {
// var weather;
var msg = "no response";
var req = new XMLHttpRequest();
var ipAddress = "158.130.110.205"; // Hard coded IP address
var port = "3000"; // Same port specified as argument to server
var url = "http://" + ipAddress + ":" + port + "/c";
  
//   if (current_temp == 0){
//     url = "http://" + ipAddress + ":" + port + "/c";
//   }

  if (celsius == 1){
    url = "http://" + ipAddress + ":" + port + "/f";
  }
  
  if (current_temp == 1){
    url = "http://" + ipAddress + ":" + port + "/t" + api_temp;
  }
  
  if (standby == 1){
    url = "http://" + ipAddress + ":" + port + "/s";
  }
var method = "GET";
var async = true;
req.onload = function(e) {
 // see what came back
if (req.status != 200){
  msg = 'server error';
  
} else {
  var response = JSON.parse(req.responseText);
if (response) {
  if (response.arduino){
    msg = response.arduino;
  }else if (response.name && response.avg && response.min && response.max && response.currentF && response.avgF && response.minF && response.maxF) {
   
   if (standby == 1){
     msg = "stand by";
   } else {
     if (current_temp == 1){
        msg = 'Temperature of current location: ';
        msg += api_temp;
        msg += 'C';
     } else {
       if (celsius == 0){
           msg = "Current: ";
           msg += response.name;
           msg += "\nAverage: ";
           msg += response.avg;
           msg += "\nMin: ";
           msg += response.min;
           msg += "\nMax: ";
           msg += response.max;
     } else{
         msg = "CurrentF:";
         msg += response.currentF;
         msg += "\nAverageF:";
         msg += response.avgF;
         msg += "\nMinF:";
         msg += response.minF;
         msg += "\nMaxF:";
         msg += response.maxF;
       }
  }
   }
   
   if (response.name > 30) {
     msg += "\nH";
   }
   
 }
}

}

// sends message back to pebble
Pebble.sendAppMessage({"0": msg });

};
req.onerror = function(e){
  msg = "server down";
  Pebble.sendAppMessage({"0": msg });
};


 req.open(method, url, async);
 req.send(null);
}