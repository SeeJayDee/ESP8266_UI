/*
  Board params: 1MB flash. QIO.
  Serial Settings: 115200 baud; CR only.
*/

/*
 * TO-DO LIST
 * > must periodically check whether it is still connected (DCHP lease timeout, or AP fault)
 * >> reconnect if necessary
 * >> suggest making separate function for connecting to WiFi?
 * 
 * > MUCH MUCH LATER
 * >> implement timer. RTC? Or get time from timeserver?
 * >> alternatively have a dedicated control interface on the network...? (ie list all relays and give options to set on/off times)
 * 
 * 
 * ?? GPL license??

*/

/*
 CREDITS
 * Thanks to Marco Schwartz for his ongoing development of the aREST libraries.
 */
 
/* Uncomment the following line to enable printing of debug information to the 
 * serial terminal.
 */
#define DEBUG_MODE 1

// Includes
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <aREST.h>
#include <aREST_UI.h>

// Global Variables
char ssid[32];
char password[64];
int timeElapsed; // accessible by aREST API
unsigned long timeCheck = 0;

// Create aREST instance
aREST_UI rest = aREST_UI();

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Define which GPIO pin the relay will be connected to.
#define RELAY_PIN 2


// Forward declarationss
int relayControl(String);
void wifiConfig(String);
void wifiConnect(String);
void handleCommands();



void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  Serial.println("");

  // Connect to WiFi
  wifiConnect("systemCall");
  
  // Init variables and expose them to REST API
  timeElapsed = 0;
  rest.variable("timeElapsed",&timeElapsed);

  // Function to be exposed
  rest.function("relay",relayControl);

  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("esp8266");

  // Add title and button linked to RELAY_PIN, initialize to OFF
  rest.title("Relay Control");
  rest.button(RELAY_PIN);  
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  
timeElapsed = millis() / 1000;

  // Handle any commands received via Serial
  if (Serial.available() > 0){
    handleCommands();}


  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
#ifdef DEBUG_MODE
    Serial.println("Help! I'm stuck in a loop!");
#endif    
  }
  rest.handle(client);
  
}

void handleCommands(){
    String received = Serial.readStringUntil('\r');
    
    if (received.startsWith("relay", 0)){
      received.remove(0,6);
      relayControl(received);
    }

    else if (received.startsWith("wificonfig", 0)){
      received.remove(0,11);
      wifiConfig(received);
    }
    
    else if (received.startsWith("wificonnect", 0)){
      wifiConnect("userCall");
    }

    else {
      Serial.print("\nUnknown command: \"");
      Serial.print(received);
      Serial.println("\"\nSupported commands: relay, wificonfig, wificonnect, ");
    }
}

// Custom function accessible by the API
int relayControl(String command) {

  // Get state from command
  int state = command.toInt();

  digitalWrite(RELAY_PIN,state);
  Serial.print("Relay state: ");
  Serial.println(state);
  
  return 1;
}

void wifiConfig(String command){
/*
 * Alter WiFi config via serial command
 */
  
  // check command received
  if (command == "get"){
    // print current wifi settings
    Serial.print("\nwificonfig get\nCurrent WiFi settings:\nSSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
  }
  else if (command == "set"){
    // prompt user to input new wifi settings. Save to EEPROM and prompt to reset.
    // declare input 'String' objects
    String ssidIn;
    String passwordIn;
    int i;

    // allocate memory for String buffer
    ssidIn.reserve(32);
    passwordIn.reserve(64);
    
    Serial.println("\nwificonfig set\nPlease enter new SSID...");
    Serial.setTimeout(30000);
    ssidIn = Serial.readStringUntil('\r');
    if (ssidIn == ""){
      Serial.println("No ssid entered or timed out. Aborting...");}
    else {
      if (ssidIn.length() > 32){
        ssidIn.remove(33);}
      Serial.println("Please enter new password...");
      Serial.setTimeout(60000);
      passwordIn = Serial.readStringUntil('\r');
      if (passwordIn.length() > 63){
        passwordIn.remove(64);}

      // begin EEPROM stuff
      EEPROM.begin(96);
      // write a 0 to all 96 bytes of the EEPROM
      for (i = 0; i < 96; i++){
        EEPROM.write(i, 0);}
      EEPROM.commit();

      // write SSID to EEPROM (first 32 bytes)
      for (i = 0; i < ssidIn.length(); i++){
        EEPROM.write(i, ssidIn.charAt(i));}

      // write password to EEPROM (last 64 bytes)  
      for (i = 32; i < (passwordIn.length() + 32); i++){
        EEPROM.write(i, passwordIn.charAt(i - 32));}
      EEPROM.commit();

      // read back data from EEPROM and print
      for (i = 0; i < 32; i++){
        ssid[i] = EEPROM.read(i);}
      Serial.print("New SSID: ");
      Serial.println(ssid);

      for (i = 0; i < 64; i++){
        password[i] = EEPROM.read(i + 32);}
      Serial.print("New password: ");
      Serial.println(password);
      Serial.println("\n\nSettings saved! Please reset the device or send \"wificonnect\".");

      EEPROM.end();
      Serial.setTimeout(1000);
    }
  }
  else {
    // incorrect syntax. berate user.
    Serial.print("\nIncorrect syntax: \"wificonfig ");
    Serial.print(command);
    Serial.println("\".");
    Serial.println("Correct usage: \"wificonfig get\" or \"wificonfig set\".");
  }
}

void wifiConnect(String callType){
  /* wifiConnect() performs the following:
   *  Recalls stored WiFi config data
   *  Attempts to connect to the network specified in the config data
   *  Upon success, Starts the WiFiServer
   *  
   *  The routine that checks whether the user has sent a "Q" could be moved into
   *  the routine that checks the connection status, to allow users to cancel a 
   *  connection attempt at any time.
   */
  String received = "";
  if (callType == "systemCall"){
    // initialize char arrays
    for (int i = 0; i < 32; i++){
      ssid[i] = 0;
      password[i] = 0;
      password[i + 32] = 0; // password is 64 bytes in size
    }
      
    // recall stored WiFi data from EEPROM
    EEPROM.begin(96);
    for (int i = 0; i < 32; i++){
      ssid[i] = EEPROM.read(i);
      password[i] = EEPROM.read(i + 32);
      password[i + 32] = EEPROM.read(i + 64);
      }
    EEPROM.end(); 
    
    // Wait 5s for cancel command, then connect to WiFi
    timeCheck = millis();
    int i = 1;
    Serial.println("Send \"Q\" to cancel connection attempt.\nConnecting in 5...");
    do {
      if (Serial.available() > 0){
        received = Serial.readStringUntil('\r');
        received.toUpperCase();
        if (received == "Q"){
          Serial.println("Connection aborted.");
          break;
        }
      }
      if ((timeCheck + 1000*i) < millis()){
        Serial.print(5-i);
        if(i != 5){
          Serial.print("...");}
        i++;
      }
    } while (i < 6);
  }
  
  // if no cancellation command was received, connect to the network
  if (received != "Q"){
    WiFi.begin(ssid, password);
    Serial.print('\n');
    timeCheck = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (millis() > timeCheck + 60000){
        Serial.println("\nCONNECTION FAILED");
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED){
      Serial.println("");
      Serial.print("WiFi connected to ");
      Serial.println(ssid);

#ifdef DEBUG_MODE
      // only print passwords in debug mode      
      Serial.print("password: ");
      Serial.println(password);
#endif      

      // Start the server
      server.begin();
      Serial.println("Server started");
  
      // Print the IP address
      Serial.println(WiFi.localIP());
    }
  } 


}

