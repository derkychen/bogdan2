int testPin = 13;

void setup() {
  Serial.begin(9600);
  pinMode(testPin, OUTPUT);
}

void loop() {
  digitalWrite(testPin, HIGH);
  Serial.println("Wrote HIGH.");
  delay(1);

  digitalWrite(testPin, LOW);
  Serial.println("Wrote LOW.");
  delay(49);
}
