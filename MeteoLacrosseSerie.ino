#include <WS2300_Serial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>

WS2300_Serial WS2300 = WS2300_Serial(Serial1);

/*********/
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress serverWunder(38,102,136,125); // http://weatherstation.wunderground.com/

String id = "IPROVENC66";
String PASSWORD = "81163393";
#define UPDATE_INTERVAL            60000    // if the connection is good wait 60 (300000) seconds before updating again - should not be less than 5
unsigned long update=0;
boolean upload=1;

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
};
// Wrap the datastream into a feed
CosmFeed feed(FEED_ID, datastreams, 5 /* number of datastreams */);
CosmClient cosmclient(client);
/*********/

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello Ethernet Enabled WS2355!");
  
  if (Ethernet.begin(mac) == 0) 
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(1000);
    asm volatile ("  jmp 0"); //soft reset!  Replace with watchdog later...
  }
}
 
void loop()
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
        pubblica();
 //       wdt_reset();
        //Serial.println("tempo impiegato per fare la pubblicazione: ");
        Serial.println("Time taken to do the publication: ");
        Serial.println(millis()-update);
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

        //client.print("&indoorhumidity=");
        //client.print(temp);
        
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