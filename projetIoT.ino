#include <WiFi.h>
#include <PubSubClient.h>
#include <stdlib.h>


const char* ssid = "iPhone";
const char* password = "12345678";

// MQTT Broker IP address:
const char* mqtt_server = "172.20.10.3";



WiFiClient espClient;
PubSubClient client(espClient);

// defines pins numbers
const int trigPin = 9;
const int echoPin = 10;
const int irPin = 11;
int presence_front_montant_ir = -1;
int presence_front_montant_us = -1;
int buffer_ir = 0;
int buffer_us = 0;


int nombre_personnes = 0;

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(115200); // Starts the serial communication
  pinMode(irPin, INPUT);
  digitalWrite(irPin, HIGH);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  detecte_passage_ir();
  detecte_passage_us();
  delay(100);
  if (buffer_ir){
    buffer_ir --;
  } 
  if (buffer_us){
    buffer_us --;
  }
  
  if (buffer_us and buffer_ir){
    if (buffer_us > buffer_ir){
      Serial.println("People in");
      nombre_personnes ++;
    }
    else if (buffer_us < buffer_ir){
      Serial.println("People out ");
      nombre_personnes --;
    }
    else {
      Serial.println("People indetermined");
    }
    buffer_us = 0;
    buffer_ir = 0;
    Serial.print("Total people : ");
    Serial.println(nombre_personnes);
    send_val_to_serv("pers", nombre_personnes);
  }

  // for (int i=0; i<BUFFERLEN/2+1;i++) add_point();
  // view_array();
  // nombre_personnes += detecter_fronts();
  // send_val_to_serv("pers", nombre_personnes);

  // Serial.print("Nombre de personnes : ");
  // Serial.println(nombre_personnes);
}

void send_val_to_serv(char *topic, int distance){
//Envoie dist
  char distanceString[64];
  dtostrf(distance, 2, 2, distanceString);
  client.publish(topic, distanceString);
}

int get_distance(){
  int distance;
  long duration;

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2; // Calculating the distance
  return medianFilter(distance);

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // Exemple pour recevoir des message sur la carte
  // if (String(topic) == "esp32/output") {
  //   if(messageTemp == "xm"){
  //     Serial.println("Ouverture EV 1, 2");
  //   }
  // }

}


int detecte_passage_ir(){
  if (!digitalRead(irPin) && presence_front_montant_ir == -1){
    presence_front_montant_ir = 1;
    buffer_ir = 50;
    Serial.println("IR up");
    return presence_front_montant_ir;
  }
  if(digitalRead(irPin) && presence_front_montant_ir == 1){
    presence_front_montant_ir = -1;
    return presence_front_montant_ir;
  }
  return 0;
}

int detecte_passage_us(){
  if (get_distance()<50 && presence_front_montant_us == -1){
    presence_front_montant_us = 1;
    buffer_us = 50;
    Serial.println("US up");
    return presence_front_montant_us;
  }
  if(get_distance()>100 && presence_front_montant_us == 1){
    presence_front_montant_us = -1;
    return presence_front_montant_us;
  }
  return 0;
}

#define MEDIAN_WINDOW 5
long medianBuffer[MEDIAN_WINDOW];

long medianFilter(long newDistance) {
  // Décaler les valeurs
  for (int i = MEDIAN_WINDOW - 1; i > 0; i--) {
    medianBuffer[i] = medianBuffer[i - 1];
  }
  medianBuffer[0] = newDistance;

  // Copier et trier
  long sorted[MEDIAN_WINDOW];
  memcpy(sorted, medianBuffer, sizeof(medianBuffer));
  for (int i = 0; i < MEDIAN_WINDOW - 1; i++) {
    for (int j = i + 1; j < MEDIAN_WINDOW; j++) {
      if (sorted[i] > sorted[j]) {
        long temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }
  // Retourner la médiane
  return sorted[MEDIAN_WINDOW / 2];
}

/*

const int BUFFERLEN = 10;
const int VALUES_SIZE = 20;
int values_array[BUFFERLEN] = {0};
long temps_array[BUFFERLEN] = {0};


void add_point(){
  for (int j = 0;j<(BUFFERLEN - 1);j++){
    values_array[j] = values_array[j+1];
    temps_array[j] = temps_array[j+1];
  }
  
  // int sum = 0;
  // // Serial.print("Values : [");
  // for (int j = 0;j<VALUES_SIZE;j++){
  //   int new_distance;
  //   new_distance = get_distance();
  //   // Serial.print(new_distance);
  //   // Serial.print(", ");
    
  //   sum = sum + get_distance();
  //   delay(3);
  // }
  // Serial.print("]");
  //values_array[BUFFERLEN - 1] = sum/VALUES_SIZE; 
  long dist = medianFilter(get_distance());
  while (dist > 300) dist = medianFilter(get_distance());

  values_array[BUFFERLEN - 1] = dist;
  temps_array[BUFFERLEN - 1] = millis();
  delay(5);

  send_val_to_serv("distance", values_array[BUFFERLEN - 1]);
}

void view_array(){
  Serial.print("\n");
  for (int j = 0;j<BUFFERLEN;j++){
    Serial.print(values_array[j]);
    Serial.print(" ");
  }
  Serial.print("\n");
}

int detecter_fronts(){
  int seuil = 100;
  int seuilFort = -20;
  int seuilFaible = -3;


  if (values_array[9] < seuil){
    int diff = 0;
    int diffTemps = 0;
    for (int i = 1 ; i < 9; i++) diff += (values_array[i] - values_array[i-1]) / (temps_array[i] - temps_array[i-1]);
    
    diff /= 7;
    // Serial.println(diff);
    // send_val_to_serv("diff", diff);

    if (diff < seuilFort){
      delay(1000);
      return 1;
    } 
    else if  (diff < seuilFaible){
      delay(1000);
      return -1;
    } 
    else return 0;
  }
  return 0;
}

*/