int analog_pin = A5;
int val = 0;
float val_V = 0.0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(analog_pin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  val = analogRead(analog_pin);
  val_V = val * (5.0 / 1024.0);
  Serial.println("Measured voltage is " + (String)val_V +" V");
  delay(200);
}
