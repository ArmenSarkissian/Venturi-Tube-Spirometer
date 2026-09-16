#define SENSOR_PIN 34  
#define LED_PIN 2     

const float D1 = 0.020;  
const float D2 = 0.013;  
const float rho = 1.204; 

const float area1 = (3.14159 / 4.0) * D1 * D1;
const float area2 = (3.14159 / 4.0) * D2 * D2;

const float DIVIDER_MULTIPLIER = 1.5;
const float V_SPAN_POSITIVE = 2.0; 
const float P_MAX_PA = 5000.0;     
const float MAX_FLOW_LMIN = 500.0; 

float vZero = 2.5; 

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  pinMode(LED_PIN, OUTPUT);

  // Dynamic Zero Offset Calibration (Tare)
  float voltageSum = 0.0;
  for (int i = 0; i < 50; i++) {
    int raw = analogRead(SENSOR_PIN);
    float pinV = (raw / 4095.0) * 3.3;
    voltageSum += pinV * DIVIDER_MULTIPLIER;
    delay(20);
  }
  vZero = voltageSum / 50.0;
}

void loop() {
  int rawADC = analogRead(SENSOR_PIN); 
  float pinVoltage = (rawADC / 4095.0) * 3.3; 
  float sensorVoltage = pinVoltage * DIVIDER_MULTIPLIER; 

float deltaP_Pa = ((sensorVoltage - vZero) / V_SPAN_POSITIVE) * P_MAX_PA;
if (deltaP_Pa < 0.0) deltaP_Pa = 0.0;

  float areaRatioSq = pow(area2 / area1, 2);
  float velocity_throat = sqrt((2.0 * deltaP_Pa) / (rho * (1.0 - areaRatioSq)));
  float flowRate_Lmin = (area2 * velocity_throat) * 60000.0; 

// --- Adjusted LED Brightness Scaling ---
const float VISIBLE_MAX_FLOW = 50.0; // Max flow target for 100% LED brightness (L/min)

// Scale flow rate directly from 0.0 to 50.0 L/min up to 0-255 PWM
int pwmBrightness = (int)constrain((flowRate_Lmin / VISIBLE_MAX_FLOW) * 255.0, 0.0, 255.0);

analogWrite(LED_PIN, pwmBrightness);

  Serial.print("Sensor_V:");       Serial.print(sensorVoltage);      Serial.print(",");
  Serial.print("Pressure_kPa:");  Serial.print(deltaP_Pa / 1000.0); Serial.print(",");
  Serial.print("FlowRate_Lmin:"); Serial.println(flowRate_Lmin);

  delay(50);
}