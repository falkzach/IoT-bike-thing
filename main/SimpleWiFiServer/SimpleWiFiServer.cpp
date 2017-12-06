

/*
 * Rui Santos 
 * Complete Project Details http://randomnerdtutorials.com
*/

#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "AndroidAP";
const char* password = "aeiou&^Y";

WiFiServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;
void print_html(WiFiClient client) {
  const char HTML[] = R"======(
  <input type="file" id="files" name="files[]" multiple />
  <div id="floating-panel">
    <button onclick="toggleKmlLayer()">Toggle KML</button>
    <button onclick="toggleHeatmap()">Toggle Heatmap</button>
    <button onclick="changeGradient()">Change gradient</button>
    <button onclick="changeRadius()">Change radius</button>
  </div>
  <div id="map"></div>
  <div id="lg"></div>)======";
  client.println(HTML);
}

void print_js(WiFiClient client) {
  const char HTML[] = R"======(
    <script>

    var map, heatmap, kmlLayer, reader, parser, xmlDoc;

    var heatmapPoints = [];

    parser = new DOMParser();

    function initMap() {
      map = new google.maps.Map(document.getElementById('map'), {
        zoom: 13,
        center: {lat: 46.858574, lng: -114.012864},
        mapTypeId: 'satellite'
      });

      heatmap = new google.maps.visualization.HeatmapLayer({
        data: [],
        map: map
      });

      kmlLayer = new google.maps.KmlLayer({
        url: 'https://sites.google.com/site/kmlrepobikething/kml_files/kootenai_round_2.kml',
        suppressInfoWindows: true,
        map: null 
      });

      kmlLayer.addListener('click', function(kmlEvent) {
        var text = kmlEvent.featureData.info_window_html;
        showInContentWindow(text);
      });
  
  console.log(kmlLayer.getMap());

      function showInContentWindow(text) {
        var sidediv = document.getElementById('lg');
        sidediv.innerHTML = text;
      }
    }

    function toggleHeatmap() {
      heatmap.setMap(heatmap.getMap() ? null : map);
    }

    function toggleKmlLayer() {

        kmlLayer.setMap(kmlLayer.getMap() ? null : map);

    }


    function changeGradient() {
      var gradient = [
        'rgba(0, 255, 255, 0)',
        'rgba(0, 255, 255, 1)',
        'rgba(0, 191, 255, 1)',
        'rgba(0, 127, 255, 1)',
        'rgba(0, 63, 255, 1)',
        'rgba(0, 0, 255, 1)',
        'rgba(0, 0, 223, 1)',
        'rgba(0, 0, 191, 1)',
        'rgba(0, 0, 159, 1)',
        'rgba(0, 0, 127, 1)',
        'rgba(63, 0, 91, 1)',
        'rgba(127, 0, 63, 1)',
        'rgba(191, 0, 31, 1)',
        'rgba(255, 0, 0, 1)'
      ]
      heatmap.set('gradient', heatmap.get('gradient') ? null : gradient);
    }

    function changeRadius() {
      heatmap.set('radius', heatmap.get('radius') ? null : 25);
    }

    function handleFileSelect(evt) {
      files = evt.target.files;

      // Loop through the FileList
      for (var i = 0, f; f = files[i]; i++) {

        var reader = new FileReader();

        // Closure to capture the file information.
        reader.onload = (function(theFile) {
          return function(e) {
            // Print the contents of the file
            var span = document.createElement('span');
            span.innerHTML = e.target.result;
            parseKML(span.innerHTML);
            console.log(span.innerHTML);
          };
        })(f);

        reader.readAsText(f);
      }
    }

    function parseKML(text) {
      
  xmlDoc = parser.parseFromString(text, "text/xml");
  
  // find all placemarks
  var size = xmlDoc.getElementsByTagName("placemark").length;
  
  if (size > 0) {
        
    var coordinates = xmlDoc.getElementsByTagName("coordinates");
          
    if (xmlDoc.getElementsByTagName("extendeddata").length > 0) {
            
      var magnitudes = xmlDoc.getElementsByTagName("value");
              
      for (var p = 0; p < size; ++p) {
                
                  var coor = coordinates[p].childNodes[0].nodeValue.split(',');
                  var mag = xmlDoc.getElementsByTagName("value")[2*p+1].childNodes[0].nodeValue;
                
                  heatmapPoints.push({location: new google.maps.LatLng(coor[1], coor[0]), weight: mag});
                
              }
                
    } else {
  
      coordinates = (coordinates[0].firstChild.nodeValue.replace( /\n/g, " " ).split( " " ));
  
      for (var i = 0; i < coordinates.length; ++i) {
                  
        coor = coordinates[i].split(',');
                  
        if (coor.length > 1) {
                
                      heatmapPoints.push(new google.maps.LatLng(coor[1], coor[0]));
                  }
              }
    }
    
    heatmap.setData(heatmapPoints);
        }
    }
    
    document.getElementById('files').addEventListener('change', handleFileSelect, false);

  </script>
  <script async defer
    src="https://maps.googleapis.com/maps/api/js?key=AIzaSyAKe8A9vDTluZYfwHZAsJmepp46QJr2gIc&libraries=visualization&callback=initMap">
  </script>)======";
client.println(HTML);
}
static void init_wifi() {
  
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
 WiFi.begin(ssid, password); 

  // attempt to connect to WiFi network:
//  while (WiFi.status() != WL_CONNECTED) {
//    Serial.println(WiFi.status());
//    delay(100);
//    
// }
}

static void listen(void* arg) {
  // listen for incoming clients
  while (WiFi.status() != WL_CONNECTED) {
	Serial.print(WiFi.status());
	delay(500);
  }
	server.begin(); // start the web server on port 80
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><head>");
          client.println("<style>#map{height: 90%;margin: auto;width: 90%;border: 2px solid blue;}html, body {height: 100%;margin: 0;padding: 0;}#floating-panel {position: absolute;top: 10px;left: 25%;z-index: 5;background-color: #fff;padding: 5px;border: 1px solid #999;text-align: center;fontfamily: 'Roboto','sans-serif';line-height: 30px;padding-left: 10px;}</style>");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
     
          print_html(client);
          print_js(client);
          client.println("</body></html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
         // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}


