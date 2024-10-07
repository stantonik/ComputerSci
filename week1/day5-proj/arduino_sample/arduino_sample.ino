
int i = 0;

void setup() {
  Serial.begin(9600);
}

void loop() 
{
  if (i > 100) i = 0;
  Serial.println(i);
  i+=1;
  delay(100);
}
