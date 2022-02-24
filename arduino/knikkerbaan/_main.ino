#include <Arduino_JSON.h>
#include <Servo.h>

Servo servo360;

KnikkerPoort poortBoven = KnikkerPoort();
Wissel wisselPoort = Wissel();
WiFiCommunicator wifi = WiFiCommunicator(WIFI_NETWERK, WIFI_WACHTWOORD, SERVER_DOMEINNAAM);
Teller tellerA = Teller(TELLER_A_PIN);
Teller tellerB = Teller(TELLER_B_PIN);

int serverContactInterval = 15; // 15 seconden
int oudAantalKnikkers = 0;
int tijdKnikkerInDoosje = 3000;
bool wisselRechts = false;
bool knikkerWaargenomen = false; 
unsigned long tijdVoorContactMetServer = 0;
unsigned long tijdWisselScheef = 0;


void setup() {
  Serial.begin(9600);
  
  poortBoven.begin(BOVEN_POORT_PIN, 0, 90);
  wisselPoort.begin(WISSEL_PIN,0, 90, 45);
  
  //wifi.begin();

  //wifi.stuurVerzoek("/api/set/nieuwerun", "");

  poortBoven.open();
  wisselPoort.midden();

  servo360.attach(3);

}


void loop() {
  // laat de teller detecteren:
  tellerA.update();
  tellerB.update();

  
  // pauzeer de knikkerbaan als het tijd is voor contact met server
  if (millis() > tijdVoorContactMetServer && poortBoven.getOpen()) {
    poortBoven.sluit();
  }

  // knikkerbaan is leeggelopen, er zijn geen sensors dit iets moeten meten
  // nu is het tijd om contact te leggen met de server:
  if (millis() > tijdVoorContactMetServer + LEEGLOOP_TIJD) {
    Serial.print("Er zijn nu zoveel knikkers geteld bij sensor 1: ");
    Serial.println(tellerA.getAantal());
    Serial.print("Er zijn nu zoveel knikkers geteld bij sensor 2: ");
    Serial.println(tellerB.getAantal());

    String data = "knikkers=";
    data += tellerA.getAantal();

    wifi.stuurVerzoek("/api/set/sensordata", data. c_str());

    String serverAntwoord = wifi.stuurVerzoek("/api/get/instellingen", "");
    Serial.println(serverAntwoord);

    JSONVar ontvangenInstellingen = JSON.parse(serverAntwoord);

    if(JSON.typeof(ontvangenInstellingen != "undefined")){
      //nieuwe wachttijd instellen:
      serverContactInterval = (int)ontvangenInstellingen["wachttijdPoort"];
      wisselRechts = (int)ontvangenInstellingen["wisselStaatRechts"];
    }
    else{
      Serial.println("FOUT: serverAntwoord kon niet worden verwerkt");
    }

    tijdVoorContactMetServer = millis() + (unsigned long)serverContactInterval * 1000;

    if (wisselPoort.getMidden() && tellerB.getAantal() > oudAantalKnikkers) {
      Serial.println("Knikker waargenomen");
      Serial.println(tellerB.getAantal());
      knikkerWaargenomen = true;
    }

    if (knikkerWaargenomen == true) {
      Serial.println("tijd in doosje wordt bepaald");
      tijdKnikkerInDoosje = millis() + 3000;
      if (millis() >  tijdKnikkerInDoosje) {
        Serial.println("tijd in doosje is voorbij");
        if (wisselRechts == true) {
          wisselPoort.rechts();
        }
        else {
          wisselPoort.links();
        }
        tijdWisselScheef = millis() + 3000;
        if (millis() > tijdWisselScheef && !wisselPoort.getMidden()) {
          Serial.println("wissel gaat weer sluiten");
          wisselPoort.midden();
        }
      }
      knikkerWaargenomen = false;
      oudAantalKnikkers = oudAantalKnikkers + 1;
    }
      
    // en zet nu het poortje weer open:
    poortBoven.open();

     servo360.write(80);

  }
}
