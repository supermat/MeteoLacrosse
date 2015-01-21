#include <WS2300_Serial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>
#include <SD.h>

#define LOGGING;

WS2300_Serial WS2300 = WS2300_Serial(Serial1);

/*********/
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress serverWunder(38,102,136,125); // http://weatherstation.wunderground.com/
String id = "IPROVENC66";
String PASSWORD = "81163393";
#define UPDATE_INTERVAL            60000    // if the connection is good wait 60 (300000) seconds before updating again - should not be less than 5
unsigned long update=0;
boolean upload=1;

/*********ServeurWeb**********/
#define REQ_BUF_SZ   20
IPAddress ip(192, 168, 0, 20); // IP address, may need to change depending on network
IPAddress gateway(192,168,0,254); // internet access via router
IPAddress subnet(255,255,255,0); 
EthernetServer server(84);  // create a server at port 80
File webFile;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
/***************/

EthernetClient client;
/*********/
#define API_KEY "7D0kjWbqrg7yW-MiAsPH11PXkfiSAKxuc0VTbWdvUmJGZz0g" // your Cosm API key
#define FEED_ID 129635 // your Cosm feed ID
// Define the string for our datastream ID
CosmDatastream datastreams[] = {
  CosmDatastream("temp_ext", 8, DATASTREAM_STRING),
  CosmDatastream("temp_int", 8, DATASTREAM_STRING),
  CosmDatastream("hum_ext", 7, DATASTREAM_STRING),
  CosmDatastream("hum_int", 7, DATASTREAM_STRING),
  CosmDatastream("pression", 8, DATASTREAM_STRING),
  CosmDatastream("watt_sapin", 10, DATASTREAM_STRING),
  CosmDatastream("watt_lavelinge", 14, DATASTREAM_STRING)
};
// Wrap the datastream into a feed
CosmFeed feed(FEED_ID, datastreams, 7 /* number of datastreams */);
CosmClient cosmclient(client);
/*********/


/**********/
IPAddress smartplugSapin(192 , 168 , 0 , 40); 
IPAddress smartplugLaveLinge(192 , 168 , 0 , 41); 
String Ivalue ;
String Wvalue ;
String Vvalue ;
/**********/

void setup()
{

  
Serial.begin(115200);
Serial.println("Hello Ethernet Enabled WS2355!");
  
  // initialize SD card
Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
Serial.println("SUCCESS - Found index.htm file.");
  //Ethernet.begin(mac,ip);
  //Ethernet.begin(mac, ip, subnet, gateway); => COSM don't work with this config
  if (Ethernet.begin(mac) == 0) 
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(1000);
    asm volatile ("  jmp 0"); //soft reset!  Replace with watchdog later...
  }
  server.begin();
  Serial.print("Ip Local : ");
  Serial.println(Ethernet.localIP());
}
 
void loop()
{
  int incomingByteSer;      // a variable to read incoming serial data into

  listenSerie();
  listenWeb();
  
  if (upload==1)
  {
    if (millis() < update) update = millis();
    
    if ((millis()% 1000) < 2)
    {
      delay (100);
      Serial.print(".");
    }
    if ((millis() - update) > UPDATE_INTERVAL)
    {
        update = millis();
 //       wdt_reset();
        String wattSapin = GetPlugWatt(smartplugSapin);
        datastreams[5].setString(wattSapin);
        Serial.print("wattSapin: ");
        Serial.println(wattSapin);
        delay(100);
        String wattLaveLinge = GetPlugWatt(smartplugLaveLinge);
        datastreams[6].setString(wattLaveLinge);
        Serial.print("wattLaveLinge: ");
        Serial.println(wattLaveLinge);
        //listenSmartplug(smartplugSapin);
        pubblica();
 //       wdt_reset();
        //Serial.println("tempo impiegato per fare la pubblicazione: ");
        Serial.println("Time taken to do the publication: ");
        Serial.println(millis()-update);
    }
  }
}

void listenSmartplug(IPAddress p_IP)
{
  if(client.connect(p_IP,23)) // making telnet connetion to plug
  {
    // Serial.println("Connected");
    delay(200);
    client.println("admin"); // user admin
    delay(200);
    client.println("admin"); // password admin
    delay(1000);
    client.flush();
    Vvalue = GetPlugData("GetInfo V");
    Wvalue = GetPlugData("GetInfo W");
    Ivalue = GetPlugData("GetInfo I");
    
    String watt = Wvalue.substring(0,4) + "." + Wvalue.substring(4);
    
    Serial.print("Voltage : ");
    Serial.print(Vvalue);
    Serial.print(" Wattage : ");
    Serial.println(watt);
    Serial.print(" Intensité : ");
    Serial.println(Ivalue);
    
    // you can also use GetInfo I , for the amps
    
    client.flush();
    client.stop();
  }
}

void listenSerie()
{
  int incomingByteSer;      // a variable to read incoming serial data into
  // see if there's incoming serial data:
  if (Serial.available() > 0)
  {
    // read the oldest byte in the serial buffer:
    incomingByteSer = Serial.read();
    //Serial.flush();
    
    if (incomingByteSer == 'a') 
    {
      if (upload==1)
      {
        Serial.println("a=Turn off the automatic upload ");
        upload=0;
      }
      else 
      {
        Serial.println("a=Turn on the automatic upload ");
        upload=1;
      }
    }
    if (incomingByteSer == 'u') 
    {
      Serial.print("u=Publish Data every 1 min ");
      pubblica();
    }
    
    if (incomingByteSer == 'o') 
    {
      Serial.print("o=Time ");
      String data=WS2300.getTime();
      Serial.println(data);
    }  
 
    if (incomingByteSer == 'd') 
    {
      Serial.print("d=Day ");
      String data=WS2300.getDay();
      Serial.println(data);
    }
    
    if (incomingByteSer == 't') 
    {
      Serial.print("t=Temperature internal (C) ");
      String data=WS2300.FahrenheitToCelsius(WS2300.getTemp(1));
      Serial.println(data);
      /******/
      datastreams[0].setString(data);
      Serial.println(datastreams[0].getString());
      int ret = cosmclient.put(feed, API_KEY);
      Serial.print("PUT return code: ");
      /*char carray[data.length() + 1]; //determine size of the array
      data.toCharArray(carray, sizeof(carray)); //put readStringinto an array
      datastreams[0].setFloat(atof(carray));
      Serial.print("stream");
      Serial.println(datastreams[0].getFloat());
      int ret = cosmclient.put(feed, API_KEY);
      Serial.print("PUT return code: ");
      Serial.println(ret);*/
      /******/
    } 

    if (incomingByteSer == 'T') 
    {
      Serial.print("T=Temperature external (C) ");
      String data=WS2300.FahrenheitToCelsius(WS2300.getTemp(0));
      Serial.println(data);
    } 

    if (incomingByteSer == 'h') 
    {
      Serial.print("h=Humidity internal ");
      String data=WS2300.getHum(1);
      Serial.println(data);
    } 

    if (incomingByteSer == 'H') 
    {
      Serial.print("H=Humidity external ");
      String data=WS2300.getHum(0);
      Serial.println(data);
    } 

    if (incomingByteSer == 'p') 
    {
      Serial.print("p=Pressure (hPa) ");
      String data=WS2300.getPress(1);
      Serial.println(data);
    } 

    if (incomingByteSer == 'P') 
    {
      Serial.print("P=Pressure (Hg) ");
      String data=WS2300.getPress(0);
      Serial.println(data);
    } 

    if (incomingByteSer == 'w') 
    {
      Serial.print("w=Wind Speed (mph) ");
      String data=WS2300.getWind(0);
      Serial.println(data);     
    } 

    if (incomingByteSer == 'W') 
    {
      Serial.print("W=Wind Dir ");
      String data=WS2300.getWind(1);
      Serial.println(data);
    } 

    if (incomingByteSer == 'r') 
    {
      Serial.print("r=Rain (1hr) ");
      String data=WS2300.getRain(1);
      Serial.println(data);
    } 

    if (incomingByteSer == 'R') 
    {
      Serial.print("R=Rain (24hr) ");
      String data=WS2300.getRain(0);
      Serial.println(data);
    }  

    if (incomingByteSer == 'e') 
    {
      Serial.print("e=Dew point (C) ");
      String data=WS2300.FahrenheitToCelsius(WS2300.getDew());
      Serial.println(data);
    } 
  }
}


void pubblica(){

  int timeout=0;
  int skip=0;
  String inString="";
  Serial.println();
  Serial.print("Connecting... ");
  
  if (client.connect(serverWunder, 80)) {
    Serial.println("connected");
    //client.println("GET / HTTP/1.0");
    Serial.print("GET /weatherstation/updateweatherstation.php?");//modificare qua.
    client.print("GET /weatherstation/updateweatherstation.php?");//modificare qua.
    pubbws();
    client.println(" HTTP/1.0");
    Serial.println(" HTTP/1.0");

    Serial.print("HOST: ");
    client.print("HOST: ");
    client.println("http://www.wunderground.com");
    Serial.println("http://www.wunderground.com");
    client.println();
  }
  else
  {
    Serial.println();
    Serial.println("connection failed");
  }
  
  while (!client.available() && timeout<50)
  {
    timeout++;
    Serial.println();
    Serial.print("Time out ");
    Serial.println(timeout);
    delay(100);
  }

  while (client.available())
  {
        char c = client.read();
          if ((inString.length())<150){ inString = inString + c;}
  }         
  client.flush();

  if ((inString.length())>5)
  {
          //Serial.print("Risposta ");
          Serial.println();
          Serial.print("Response: ");
          Serial.print(inString);
  }              

  if (!client.connected())
  {
        Serial.println();
        Serial.println("Disconnecting.");
  }
  
  client.stop();
  delay (1000);

// Ne fonctionne plus depuis lajout du site web
Serial.println("https://cosm.com/feeds/129635");
    int ret = cosmclient.put(feed, API_KEY);
    Serial.print("PUT return code: ");
    Serial.println(ret);
//  digitalWrite(ledPin, LOW);

}

void pubbws(){
      String temp = WS2300.getDay();
      //Serial.print(temp);
      String temp1= WS2300.getTime();
      //Serial.print(temp1);
      if ((temp.length()>6) && (temp1.length()>4)){
        Serial.print("ID=");
        Serial.print(id);
        Serial.print("&PASSWORD=");
        Serial.print(PASSWORD);
        Serial.print("&dateutc=");
        Serial.print(temp);
        Serial.print("+");
        Serial.print(temp1);
        client.print("ID=");
        client.print(id);
        client.print("&PASSWORD=");
        client.print(PASSWORD);
        client.print("&dateutc=");
        client.print(temp);
        client.print("+");
        client.print(temp1);
      }
      else
      {
        //Serial.println("Lettura data/ora non riuscita torno");
        Serial.println("Reading back data failed: Header");
        return;
      }
      temp=WS2300.getWind(1);
      if (temp.length()>1){
        Serial.print("&winddir=");
        Serial.print(temp);    

        client.print("&winddir=");
        client.print(temp);
      }    

      temp=WS2300.getWind(0);
      if (temp.length()>2){
        Serial.print("&windspeedmph=");
        Serial.print(temp);        

        client.print("&windspeedmph=");
        client.print(temp);
      }   


      temp=WS2300.getTemp(0);
      if (temp.length()>3){
        Serial.print("&tempf=");
        Serial.print(temp);

        client.print("&tempf=");
        client.print(temp);
        
        temp = WS2300.FahrenheitToCelsius(temp);
        datastreams[0].setString(temp);
        Serial.println(datastreams[0].getString());
      }   


      temp=WS2300.getTemp(1);
      if (temp.length()>3){
        Serial.print("&indoortempf=");
        Serial.print(temp);

        client.print("&indoortempf=");
        client.print(temp);
        
        temp = WS2300.FahrenheitToCelsius(temp);
        datastreams[1].setString(temp);
        Serial.println(datastreams[1].getString());
      }   


      temp=WS2300.getRain(1);
      if (temp.length()>2){
        Serial.print("&rainin=");
        Serial.print(temp);       

        client.print("&rainin=");
        client.print(temp);
      }   
/*
//Doesn't work!  Retuns 2.54?!  Issue with the mm to inch conversion I suspect...
      temp=getRain(0);
      if (temp.length()>2){
        Serial.print("&dailyrainin=");
        Serial.print(temp);       

        client.print("&dailyrainin=");
        client.print(temp);
      }   
*/
      temp=WS2300.getHum(0);
      if (temp.length()>1){
        Serial.print("&humidity=");
        Serial.print(temp);        

        client.print("&humidity=");
        client.print(temp);
        
        datastreams[2].setString(temp);
        Serial.println(datastreams[2].getString());
      }   

 
      temp=WS2300.getHum(1);
      if (temp.length()>1){
        Serial.print("&indoorhumidity=");
        Serial.print(temp);        

        client.print("&indoorhumidity=");
        client.print(temp);
        
        datastreams[3].setString(temp);
        Serial.println(datastreams[3].getString());
      }   

      temp=WS2300.getDew();
      if (temp.length()>1){
        Serial.print("&dewptf=");
        Serial.print(temp);        

        client.print("&dewptf=");
        client.print(temp);        

      }   

      temp=WS2300.getPress(0);
      if (temp.length()>1){
        Serial.print("&baromin=");
        Serial.print(temp);        

        client.print("&baromin=");
        client.print(temp);        

        temp=WS2300.getPress(1);
        datastreams[4].setString(temp);
        Serial.println(datastreams[4].getString());
      } 

      //Serial.print("&windgustmph=0.00");
      //client.print("&windgustmph=0.00");
      Serial.print("&action=updateraw");
      client.print("&action=updateraw");

}

/********************************/
/********************************/
/********************************/
void listenWeb()
{
    client = server.available();  // try to get client
    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // print HTTP request character to serial monitor
                Serial.print(c);
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                  if (StrContains(HTTP_req, "/ws/")) {
Serial.print("WebService");
                        //Web Services
                        WS(client,HTTP_req);
                  }
                  else if(StrContains(HTTP_req, "fichier"))
                  {
Serial.println("Liste Fichiers");
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println();
                    // print all the files, use a helper to keep it clean
                    //ListFiles(client, 0);
                    client.println("<h2>Files:</h2>");
                    File root = SD.open("/");
                    printDirectory(client,root, 0);
                  }
                  else if(IsWebPage(HTTP_req))
                  {
Serial.print("WebPage");
                    ShowWebPageInSD(HTTP_req);
                  }
                  else
                  {
                    Serial.print("Argument : ");
                    char* arg1 = GetFirstArg(HTTP_req);
                    Serial.println(arg1);
                    Serial.println(HTTP_req);
                    char* arg2 = GetFirstArg(HTTP_req);
                    Serial.println(arg2);
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.print("<h2>");
                    client.print(arg1);
                    client.print("</h2>");
                  }
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                }
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
        Serial.println("Stop");
    } // end if (client)
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    /*char found = 0;
    char index = 0;
    char len;

    len = strlen(str);

    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;*/
    return (strstr(str, sfind) != 0);
}

#define DATE_BUFFER_SIZE 30
#define CMDBUF 50

char* ParseUrl(char *str)
{
  int8_t r=-1;
  int8_t i = 0;
  int index = 4;
  int index1 = 0;
  int plen = 0;
  char ch = str[index];
  char clientline[CMDBUF];
  while( ch != ' ' && index < CMDBUF)
  {
    clientline[index1++] = ch;
    ch = str[++index];
  }
  clientline[index1] = '\0';

  // convert clientline into a proper
  // string for further processing
  String urlString = String(clientline);
  // extract the operation
  String op = urlString.substring(0,urlString.indexOf(' '));
  // we're only interested in the first part...
  urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));

  // put what's left of the URL back in client line
  urlString.toCharArray(clientline, CMDBUF);

  return clientline;
}

char* GetFirstArg(char *str)
{
  //static char clientline[CMDBUF];

  if(str != NULL)
  {
          sprintf(str,"%s",ParseUrl(str));
          return strtok(str,"/");
  }
  else
  {
          return strtok(NULL,"/");
  }
}

// returns 1 if Web Page
// returns 0 if not web Page
char IsWebPage(char *str)
{
  if (StrContains(str, "GET / HTTP/") || StrContains(str, ".htm") || StrContains(str, ".jpg") || StrContains(str, ".ico") || StrContains(str, ".js"))
  {
    return 1;
  }
  return 0;
}

void ShowWebPageInSD(char *str)
{
    char* v_Fichier = ParseUrl(str);
    v_Fichier = GetFirstArg(str);
    if(StrContains(str, "GET / HTTP/"))
    {
      //v_Fichier = "index.htm";
      String("index.htm").toCharArray(v_Fichier, CMDBUF);
    }
#ifdef DEBUG
Serial.print("ShowWebPageInSD ");
Serial.print("(");
Serial.print(v_Fichier);
Serial.print(")");
Serial.println(str);
#endif
    if(SD.exists(v_Fichier))
    {
      client.println("HTTP/1.1 200 OK");
      if (StrContains(v_Fichier, ".htm"))
      {
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
      }
      else if (StrContains(v_Fichier, ".jpg"))
      { //Nothing to do
      }
          client.println();
          webFile = SD.open(v_Fichier);
          if (webFile) {
             while(webFile.available()) {
                client.write(webFile.read()); // send web page to client
             }
             webFile.close();
          }
    }
    else
    {
      send404(client);
    }

}

void send404(EthernetClient client) {
     client.println("HTTP/1.1 404 OK");
     client.println("Content-Type: text/html");
     client.println("Connnection: close");
     client.println();
     client.println("<!DOCTYPE HTML>");
     client.println("<html><body>404</body></html>");
}

// send the state of the switch to the web browser
void WS(EthernetClient cl, char* p_req)
{
  Serial.println("Web Services");
  String data="-";
  if (StrContains(p_req, "tempext")){
     data=WS2300.FahrenheitToCelsius(WS2300.getTemp(0));
  }
  else if (StrContains(p_req, "tempint")){
     data=WS2300.FahrenheitToCelsius(WS2300.getTemp(1));
  }
 else if (StrContains(p_req, "humext")){
     data=WS2300.getHum(0);
 }
 else if (StrContains(p_req, "humint")){
     data=WS2300.getHum(1);
 }
 else if(StrContains(p_req, "SapinOn")){
   PriseWifi(true);
 }
 else if(StrContains(p_req, "SapinOff")){
   PriseWifi(false);
 }
 else if(StrContains(p_req, "SapinWatt")){
   data=GetPlugWatt(smartplugSapin);
 }
 else{
    data = "inconnu";
 }
 cl.println(data);
}
void printDirectory(EthernetClient cl,File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       cl.print('\t');
     }
     cl.print(entry.name());
     if (entry.isDirectory()) {
       cl.println("/");
       printDirectory(cl,entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       cl.print("\t\t");
       cl.println(entry.size(), DEC);
     }
     entry.close();
     cl.print("</br>");
   }
}

void PriseWifi(bool p_etat)
{
  client.stop(); //==> sinon connection failed 
  //IPAddress server(192,168,0,40);
  if (client.connect(smartplugSapin, 80))
  {
    Serial.println("Connection prise OK");
    client.print("GET /goform/SystemCommand?command=GpioForCrond+");
    if(p_etat)
      client.println("1");
    else
      client.println("0");
    client.println("HTTP/1.0 HOST: 192.168.0.40");
    client.println("Authorization: Basic YWRtaW46YWRtaW4=");
    client.println();
  }
  else{
    Serial.println("Connection Prise Failed");
  }
  client.flush();
  client.stop();
}

/*****************************************/
String GetPlugData(String command)
{
  client.println(command);
  delay(1000);
  command = "";
   
  while (client.available())
  {
  char c = client.read() ;
  command = command + c ;
  // Serial.print(c); // for debug
  }
  
  Serial.println(command);
  int firstdollarsign = command.indexOf('$');
  //Serial.println(firstdollarsign) ; // debug
  if(firstdollarsign >= 0)
  {
    command = command.substring(firstdollarsign+7,firstdollarsign+13);
  }
  else
  {
    command = "KO";
  }
  return (command);
}
String GetPlugWatt(IPAddress p_ip)
{
  String watt = "KO";
  if(client.connect(p_ip,23)) // making telnet connetion to plug
  {
    // Serial.println("Connected");
    delay(200);
    client.println("admin"); // user admin
    delay(200);
    client.println("admin"); // password admin
    delay(1000);
    client.flush();
    String value = GetPlugData("GetInfo W");
       
    if(value != "KO")
      watt = value.substring(0,4).toInt() + "." + value.substring(4);
    // you can also use GetInfo I , for the amps
    
    client.flush();
    client.stop();
  }
  return watt;
}

