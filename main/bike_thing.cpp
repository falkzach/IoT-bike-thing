/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include <stdio.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include "./SparkFunLSM9DS1.h"

#include "HardwareSerial.h"
#include "./TinyGPS.h"
#include <WiFi.h>

///////////////////////
// Example I2C Setup //
///////////////////////
// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M	0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG	0x6B // Would be 0x6A if SDO_AG is LOW

////////////////////////////
// Sketch Output Settings //
////////////////////////////
#define PRINT_CALCULATED
//#define PRINT_RAW
#define PRINT_SPEED 250 // 250 ms between prints
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.

LSM9DS1 imu;



HardwareSerial gps_serial(2);
//SoftwareSerial gps_serial(16,17);

TinyGPS gps;

bool recording=0, wifiMode=0;

static void smartdelay(unsigned long ms);

// Replace with your network credentials
const char* ssid     = "AndroidAP";
const char* password = "aeiou&^Y";

WiFiServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;

static unsigned long lastPrint = 0; // Keep track of print time

// Earth's magnetic field varies by location. Add or subtract 
// a declination to get a more accurate heading. Calculate 
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination

const byte recordPin = 13;
const byte wifiModePin = 12;
 
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

const byte onLedPin = 26;
const byte recordLedPin = 27;
const byte wifiLedPin = 14;

void printGyro()
{
	// Now we can use the gx, gy, and gz variables as we please.
	// Either print them as raw ADC values, or calculated in DPS.
  	Serial.print("G: ");
	#ifdef PRINT_CALCULATED
  		// If you want to print calculated values, you can use the
  		// calcGyro helper function to convert a raw ADC value to
  		// DPS. Give the function the value that you want to convert.
  		Serial.print(imu.calcGyro(imu.gx), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcGyro(imu.gy), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcGyro(imu.gz), 2);
  		Serial.println(" deg/s");
	#elif defined PRINT_RAW
  		Serial.print(imu.gx);
  		Serial.print(", ");
  		Serial.print(imu.gy);
  		Serial.print(", ");
  		Serial.println(imu.gz);
	#endif
}

void printAccel()
{
  	// Now we can use the ax, ay, and az variables as we please.
  	// Either print them as raw ADC values, or calculated in g's.	
	Serial.print("A: ");
	#ifdef PRINT_CALCULATED
		// If you want to print calculated values, you can use the
	  	// calcAccel helper function to convert a raw ADC value to
  		// g's. Give the function the value that you want to convert.
  		Serial.print(imu.calcAccel(imu.ax), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcAccel(imu.ay), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcAccel(imu.az), 2);
  		Serial.println(" g");
	#elif defined PRINT_RAW
  		Serial.print(imu.ax);
  		Serial.print(", ");
  		Serial.print(imu.ay);
  		Serial.print(", ");
  		Serial.println(imu.az);
	#endif
}

void printMag()
{
  	// Now we can use the mx, my, and mz variables as we please.
  	// Either print them as raw ADC values, or calculated in Gauss.
  	Serial.print("M: ");
	#ifdef PRINT_CALCULATED
  		// If you want to print calculated values, you can use the
  		// calcMag helper function to convert a raw ADC value to
  		// Gauss. Give the function the value that you want to convert.
  		Serial.print(imu.calcMag(imu.mx), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcMag(imu.my), 2);
  		Serial.print(", ");
  		Serial.print(imu.calcMag(imu.mz), 2);
  		Serial.println(" gauss");
	#elif defined PRINT_RAW
  		Serial.print(imu.mx);
  		Serial.print(", ");
  		Serial.print(imu.my);
  		Serial.print(", ");
  		Serial.println(imu.mz);
	#endif
}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf?fpsp=1
// Heading calculations taken from this app note:
// http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void printAttitude(float ax, float ay, float az, float mx, float my, float mz)
{
  	float roll = atan2(ay, az);
  	float pitch = atan2(-ax, sqrt(ay * ay + az * az));

  	float heading;
  	if (my == 0)
   		heading = (mx < 0) ? PI : 0;
  	else
    		heading = atan2(mx, my);

  	heading -= DECLINATION * PI / 180;

  	if (heading > PI) heading -= (2 * PI);
  	else if (heading < -PI) heading += (2 * PI);
  	else if (heading < 0) heading += 2 * PI;

  	// Convert everything from radians to degrees:
  	heading *= 180.0 / PI;
  	pitch *= 180.0 / PI;
  	roll  *= 180.0 / PI;

  	Serial.print("Pitch, Roll: ");
  	Serial.print(pitch, 2);
  	Serial.print(", ");
  	Serial.println(roll, 2);
  	Serial.print("Heading: "); Serial.println(heading, 2);
}

void print_html(WiFiClient client) {
  	const char HTML[] = R"======(<input type="file" id="files" name="files[]" multiple /><div id="floating-panel"><button onclick="toggleKmlLayer()">Toggle KML</button><button onclick="toggleHeatmap()">Toggle Heatmap</button><button onclick="changeGradient()">Change gradient</button><button onclick="changeRadius()">Change radius</button></div><div id="map"></div><div id="lg"></div>)======";
  	client.println(HTML);
}

void print_js(WiFiClient client) {
  	const char HTML[] = R"======(<script>var map, heatmap, kmlLayer, reader, parser, xmlDoc;var heatmapPoints = [];parser = new DOMParser();function initMap() { map = new google.maps.Map(document.getElementById('map'), {zoom: 13,center: {lat: 46.858574, lng: -114.012864},mapTypeId: 'satellite'}); heatmap = new google.maps.visualization.HeatmapLayer({data: [], map: map }); kmlLayer = new google.maps.KmlLayer({ url: 'https://sites.google.com/site/kmlrepobikething/kml_files/kootenai_round_2.kml', suppressInfoWindows: true, map: null }); kmlLayer.addListener('click', function(kmlEvent) { var text = kmlEvent.featureData.info_window_html; showInContentWindow(text); }); console.log(kmlLayer.getMap()); function showInContentWindow(text) { var sidediv = document.getElementById('lg'); sidediv.innerHTML = text; }}function toggleHeatmap() { heatmap.setMap(heatmap.getMap() ? null : map); } function toggleKmlLayer() { kmlLayer.setMap(kmlLayer.getMap() ? null : map); }function changeGradient() { var gradient = ['rgba(0, 255, 255, 0)','rgba(0, 255, 255, 1)','rgba(0, 191, 255, 1)', 'rgba(0, 127, 255, 1)', 'rgba(0, 63, 255, 1)', 'rgba(0, 0, 255, 1)', 'rgba(0, 0, 223, 1)','rgba(0, 0, 191, 1)', 'rgba(0, 0, 159, 1)', 'rgba(0, 0, 127, 1)', 'rgba(63, 0, 91, 1)', 'rgba(127, 0, 63, 1)', 'rgba(191, 0, 31, 1)', 'rgba(255, 0, 0, 1)' ] heatmap.set('gradient', heatmap.get('gradient') ? null : gradient); } function changeRadius() { heatmap.set('radius', heatmap.get('radius') ? null : 25); } function handleFileSelect(evt) { files = evt.target.files; for (var i = 0, f; f = files[i]; i++) { var reader = new FileReader(); reader.onload = (function(theFile) { return function(e) { var span = document.createElement('span'); span.innerHTML = e.target.result; parseKML(span.innerHTML); console.log(span.innerHTML); }; })(f); reader.readAsText(f); } } function parseKML(text) { xmlDoc = parser.parseFromString(text, "text/xml"); var size = xmlDoc.getElementsByTagName("placemark").length; if (size > 0) { var coordinates = xmlDoc.getElementsByTagName("coordinates"); if (xmlDoc.getElementsByTagName("extendeddata").length > 0) { var magnitudes = xmlDoc.getElementsByTagName("value"); for (var p = 0; p < size; ++p) { var coor = coordinates[p].childNodes[0].nodeValue.split(','); var mag = xmlDoc.getElementsByTagName("value")[2*p+1].childNodes[0].nodeValue; heatmapPoints.push({location: new google.maps.LatLng(coor[1], coor[0]), weight: mag}); } } else { coordinates = (coordinates[0].firstChild.nodeValue.replace( /\n/g, " " ).split( " " )); for (var i = 0; i < coordinates.length; ++i) { coor = coordinates[i].split(','); if (coor.length > 1) { heatmapPoints.push(new google.maps.LatLng(coor[1], coor[0])); } } }heatmap.setData(heatmapPoints); } } document.getElementById('files').addEventListener('change', handleFileSelect, false); </script> <script async defer src="https://maps.googleapis.com/maps/api/js?key=AIzaSyAKe8A9vDTluZYfwHZAsJmepp46QJr2gIc&libraries=visualization&callback=initMap"> </script>)======";
client.println(HTML);
}

void build_kml() {

}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
    	Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

void blink_on_led() {
	digitalWrite(onLedPin, HIGH);
	delay(1000);
	digitalWrite(onLedPin, LOW);
	delay(1000);
}

void recordInterrupt() {
  	portENTER_CRITICAL_ISR(&mux);
  
	if(wifiMode || recording)
		  recording = 0;
  	else
		  recording = 1;
  
  	portEXIT_CRITICAL_ISR(&mux);
}
void wifiInterrupt() {
	portENTER_CRITICAL_ISR(&mux);
  	if(recording || wifiMode)
	  	wifiMode = 0;
  	else 
	  	wifiMode = 1;
  	portEXIT_CRITICAL_ISR(&mux);
}
void setup(){
	// set up leds
	pinMode(onLedPin, OUTPUT);
    	digitalWrite(onLedPin, HIGH);

	pinMode(recordLedPin, OUTPUT);
	digitalWrite(recordLedPin, LOW);

	pinMode(wifiLedPin, OUTPUT);
	digitalWrite(wifiLedPin, LOW);	
	// set up serial ports
    	Serial.end();
	Serial.flush();
	Serial.begin(9600);
	gps_serial.begin(9600);

	pinMode(recordPin, INPUT_PULLUP);
	pinMode(wifiModePin, INPUT_PULLUP);
 	attachInterrupt(digitalPinToInterrupt(recordPin), recordInterrupt, FALLING);
	attachInterrupt(digitalPinToInterrupt(wifiModePin), wifiInterrupt, FALLING);


      	// Before initializing the IMU, there are a few settings
  	// we may need to adjust. Use the settings struct to set
  	// the device's communication mode and addresses:
  	imu.settings.device.commInterface = IMU_MODE_I2C;
  	imu.settings.device.mAddress = LSM9DS1_M;
  	imu.settings.device.agAddress = LSM9DS1_AG;
  	// The above lines will only take effect AFTER calling
  	// imu.begin(), which verifies communication with the IMU
  	// and turns it on.
  	if (!imu.begin())
  	{
  		Serial.println("Failed to communicate with LSM9DS1.");
    		Serial.println("Double-check wiring.");
    		Serial.println("Default settings in this sketch will " \
                  "work for an out of the box LSM9DS1 " \
                  "Breakout, but may need to be modified " \
                  "if the board jumpers are.");
    		while(!imu.begin()) {
			blink_on_led();
		}
		//return;
  	}

    	if(!SD.begin(5)){
        	Serial.println("Card Mount Failed");
		while(!SD.begin(5)) {
			blink_on_led();
		}
		//return;
	}
    	uint8_t cardType = SD.cardType();

    	if(cardType == CARD_NONE){
    		Serial.println("No SD card attached");
        	while (cardType == CARD_NONE){
			blink_on_led();
		}
		//return;
    	}

    	Serial.print("SD Card Type: ");
    	if(cardType == CARD_MMC){
        	Serial.println("MMC");
    	} else if(cardType == CARD_SD){
        	Serial.println("SDSC");
    	} else if(cardType == CARD_SDHC){
        	Serial.println("SDHC");
    	} else {
        	Serial.println("UNKNOWN");
    	}

    	uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    	Serial.printf("SD Card Size: %lluMB\n", cardSize);

    	createDir(SD, "/html");
    	createDir(SD, "/data");
    	listDir(SD, "/", 0);
    	// TODO change how the file is written in case there are two files, or always append to a single file if it is there.

    	writeFile(SD, "/data/gps_data.txt", "GPS HEADER\n");
    	// deleteFile(SD, "/foo.txt");
    	// renameFile(SD, "/hello.txt", "/foo.txt");
    	// readFile(SD, "/foo.txt");
    	//testFileIO(SD, "/test.txt");
    	Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    	Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void setup_wifi() {

	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);
  
	WiFi.begin(ssid, password);

	// attempt to connect to Wifi network:
	while(WiFi.status() != WL_CONNECTED && wifiMode) {
		// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
		delay(500);
		Serial.print(".");
	}
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("");
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
  
		server.begin();
	}

}

void listen_for_clients() {
	// listen for incoming clients
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

void run_sensors() {
	
	float flat, flon;
    	unsigned long age;	
	char coordinates[100];
	
	gps.f_get_position(&flat, &flon, &age);
	
	if (flat <= 90.0 && flat >= -90.0) {
		if(flon <=180 && flon >=-180) {

			sprintf(coordinates, "%f,%f\n",flat,flon);
			Serial.println(coordinates);
			appendFile(SD, "/data/gps_data.txt",coordinates);
	
			// Update the sensor values whenever new data is available
			if ( imu.gyroAvailable() ) {
				// To read from the gyroscope,  first call the
				// readGyro() function. When it exits, it'll update the
				// gx, gy, and gz variables with the most current data.
				//imu.readGyro();
			}
			if ( imu.accelAvailable() ) {
				// To read from the accelerometer, first call the
				// readAccel() function. When it exits, it'll update the
				// ax, ay, and az variables with the most current data.
				//imu.readAccel();
			}
			if ( imu.magAvailable() ) {
				// To read from the magnetometer, first call the
				// readMag() function. When it exits, it'll update the
				// mx, my, and mz variables with the most current data.
				//imu.readMag();
			}
	
			if ((lastPrint + PRINT_SPEED) < millis()) {
				//printGyro();  // Print "G: gx, gy, gz"
				//printAccel(); // Print "A: ax, ay, az"
				//printMag();   // Print "M: mx, my, mz"
				// Print the heading and orientation for fun!
				// Call print attitude. The LSM9DS1's mag x and y
				// axes are opposite to the accelerometer, so my, mx are
				// substituted for each other.
				//printAttitude(imu.ax, imu.ay, imu.az,
	        		//    -imu.my, -imu.mx, imu.mz);
				//Serial.println();
	
				lastPrint = millis(); // Update lastPrint time
			}
      			smartdelay(1000);
		}
	}
}

static void smartdelay(unsigned long ms) {
	unsigned long start = millis();
	do {
		while (gps_serial.available()) {
			const char c = gps_serial.read();
			Serial.print(c);
			appendFile(SD, "/data/gps_data.txt",&c);
			gps.encode(c);
		}
	} while (millis() - start < ms);
}

void loop(){
	

	digitalWrite(onLedPin, HIGH);
	digitalWrite(recordLedPin,LOW);
	digitalWrite(wifiLedPin, LOW);
	while(recording) {
		digitalWrite(recordLedPin, HIGH);
		run_sensors();
	}
	if(wifiMode) {
		digitalWrite(wifiLedPin,HIGH);
		// build Kml once, then loop until wifi is stopped
		setup_wifi();
		build_kml();
		//TODO put wifi code here
		while(wifiMode) {
			digitalWrite(wifiLedPin, HIGH);
			listen_for_clients();	
		}
		// tear down wifi mode: https://github.com/esp8266/Arduino/issues/644
		WiFi.disconnect();
		WiFi.mode(WIFI_OFF);
	}
	
}
