#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// =================== Wi-Fi ===================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// =================== Firebase ===================
#define FIREBASE_HOST "your-project-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRET"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// =================== Pins ===================
const int buttonPins[4] = {13, 14, 27, 26};  
const int coalitionPin = 33;                  
const int buzzerPin = 25;

// =================== Button Names ===================
const char* buttonNames[4] = {
  "Candidate_A",
  "Candidate_B",
  "Candidate_C",
  "Candidate_D"
};

// =================== Coalition Name ===================
const char* coalitionName = "Coalition_A_B"; // change name as desired

void setup() {
  Serial.begin(115200);

  // Initialize buttons
  for (int i = 0; i < 4; i++) pinMode(buttonPins[i], INPUT_PULLUP);

 
  pinMode(coalitionPin, INPUT); // no pull-up; OR gate drives HIGH/LOW

  // Initialize buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Buzz once
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);

  // Firebase setup
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.print("Connecting to Firebase");
  while (!Firebase.ready()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nFirebase ready!");
}
void handleVote(const char* candidate) {
  Serial.print(candidate);
  Serial.println(" pressed!");

  // Buzz
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);

  // Firebase vote increment
  String path = "/votes/";
  path += candidate;

  int votes = 0;
  if (Firebase.RTDB.getInt(&fbdo, path)) {
    votes = fbdo.intData();
    Serial.printf("Current votes for %s: %d\n", candidate, votes);
  } else {
    Serial.printf("Failed to get current votes for %s: %s\n", candidate, fbdo.errorReason().c_str());
  }

  votes++;

  if (Firebase.RTDB.setInt(&fbdo, path, votes)) {
    Serial.printf("Updated votes for %s: %d\n", candidate, votes);
  } else {
    Serial.printf("Failed to update votes for %s: %s\n", candidate, fbdo.errorReason().c_str());
  }
}
int lastCoalitionVotes = -1; // global variable to track previous coalition votes

void loop() {
  // ---------- Individual candidates ----------
  for (int i = 0; i < 4; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      handleVote(buttonNames[i]);
      delay(300); // debounce
    }
  }

  // ---------- Manual Coalition Calculation ----------
  int candidateAVotes = 0;
  int candidateBVotes = 0;

  // Read current votes from Firebase
  if (Firebase.RTDB.getInt(&fbdo, "/votes/Candidate_A")) {
    candidateAVotes = fbdo.intData();
  } else {
    Serial.println("Failed to read Candidate A votes: " + fbdo.errorReason());
  }

  if (Firebase.RTDB.getInt(&fbdo, "/votes/Candidate_B")) {
    candidateBVotes = fbdo.intData();
  } else {
    Serial.println("Failed to read Candidate B votes: " + fbdo.errorReason());
  }

  int coalitionVotes = candidateAVotes + candidateBVotes;

  // Print and update only if coalition votes changed
  if (coalitionVotes != lastCoalitionVotes) {
    Serial.print("Current coalition votes (Abhik + Harsh): ");
    Serial.println(coalitionVotes);

    if (!Firebase.RTDB.setInt(&fbdo, "/votes/Coalition_A_B", coalitionVotes)) {
      Serial.println("Failed to update coalition votes: " + fbdo.errorReason());
    }

    lastCoalitionVotes = coalitionVotes; // update the last value
  }

  delay(1000); // small delay to avoid flooding Firebase
}
