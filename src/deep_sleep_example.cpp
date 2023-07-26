
void setup() {
  int i;
  int minutes = 5;
  Serial.begin(115200);

  while(i<3){
    i++;
    Serial.print(".");
    delay(1000);
  }

  i = 0;

  while(i<3){
    i++;
    Serial.print("*");
    delay(1000);
  }

  i = 0;

  while(i<3){
    i++;
    Serial.print("-");
    delay(1000);
  }
  Serial.println();
  Serial.flush(); 
  int time_sleep = minutes * 60000;
  int time_running = millis();
  esp_sleep_enable_timer_wakeup((time_sleep-time_running)*1000);
  esp_deep_sleep_start();

}

void loop() {
  // put your main code here, to run repeatedly:

}