#include <EEPROM.h>

// --- VELOCIDADES SEGURAS PARA PRUEBAS ---
#define MAX_SPEED  210
#define BASE_SPEED 120   // Básico bajado para que no salga volando
#define MIN_SPEED  0     

// --- PINES DE MOTORES ---
#define speedPinR 9
#define RightMotorDirPin1 22
#define RightMotorDirPin2 24
#define LeftMotorDirPin1 26
#define LeftMotorDirPin2 28
#define speedPinL 10
#define speedPinRB 11
#define RightMotorDirPin1B 5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B 7
#define LeftMotorDirPin2B 8
#define speedPinLB 12

// --- PINES DE SENSORES Y BOTÓN ---
#define sensor1 A4 
#define sensor2 A3 
#define sensor3 A2 
#define sensor4 A1 
#define sensor5 A0 
#define PIN_BOTON 2  // Donde pinchas el extremo del Jumper (El otro a GND)

int sensorPins[5] = {sensor1, sensor2, sensor3, sensor4, sensor5};
int sThreshold[5]; 

// ================= FUZZY PID =================
// 1. Valores base (Si tiembla, baja el Kp. Si no gira, sube el Kp)
float Kp_base = 0.8; 
float Kd_base = 6.0;

// 2. Variables mutantes que cambiará el Fuzzy
float Kp_actual = 0;
float Kd_actual = 0;

float P = 0, D = 0, lastError = 0;
float filteredD = 0;
#define D_FILTER 0.3    

/* ============ Control de Motores Básico ============ */
void FR_fwd(int speed) { digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, HIGH); analogWrite(speedPinR, speed); }
void FR_bck(int speed) { digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW); analogWrite(speedPinR, speed); }
void FL_fwd(int speed) { digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, HIGH); analogWrite(speedPinL, speed); }
void FL_bck(int speed) { digitalWrite(LeftMotorDirPin1, HIGH); digitalWrite(LeftMotorDirPin2, LOW); analogWrite(speedPinL, speed); }
void RR_fwd(int speed) { digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, HIGH); analogWrite(speedPinRB, speed); }
void RR_bck(int speed) { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW); analogWrite(speedPinRB, speed); }
void RL_fwd(int speed) { digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, HIGH); analogWrite(speedPinLB, speed); }
void RL_bck(int speed) { digitalWrite(LeftMotorDirPin1B, HIGH); digitalWrite(LeftMotorDirPin2B, LOW); analogWrite(speedPinLB, speed); }

void init_GPIO() {
  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT); pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT); pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT); pinMode(speedPinRB, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP); 
}

/* ============ Control Avanzado ============ */
void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed  = constrain(leftSpeed,  -MAX_SPEED, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SPEED, MAX_SPEED);
  if (leftSpeed >= 0) { FL_fwd(leftSpeed); RL_fwd(leftSpeed); } else { FL_bck(-leftSpeed); RL_bck(-leftSpeed); }
  if (rightSpeed >= 0) { FR_fwd(rightSpeed); RR_fwd(rightSpeed); } else { FR_bck(-rightSpeed); RR_bck(-rightSpeed); }
}

/* ============ RUTINA DE CALIBRACIÓN CORTITA Y SUAVE ============ */
void calibrarSensoresAuto() {
  Serial.println("¡CALIBRANDO! Iniciando baile...");
  int sMin[5] = {1023, 1023, 1023, 1023, 1023}; 
  int sMax[5] = {0, 0, 0, 0, 0};
  unsigned long startTime = millis();
  
  // Velocidad de calibración bajada a 60 para que no gire 20 grados
  int speedCal = 60; 

  while (millis() - startTime < 3000) {  // Baile acortado a 3 seg
    unsigned long transcurrido = millis() - startTime;
    
    if (transcurrido < 800) {
      setMotors(speedCal, -speedCal); // Derecha cortito
    } else if (transcurrido < 2200) {
      setMotors(-speedCal, speedCal); // Izquierda cruzando la linea
    } else {
      setMotors(speedCal, -speedCal); // Vuelve al centro
    }

    for (int i = 0; i < 5; i++) {
      int val = analogRead(sensorPins[i]);
      if (val < sMin[i]) sMin[i] = val;
      if (val > sMax[i]) sMax[i] = val;
    }
  }

  setMotors(0, 0); // Freno en seco
  Serial.println("Guardando en EEPROM...");
  for (int i = 0; i < 5; i++) {
    sThreshold[i] = (sMin[i] + sMax[i]) / 2;
  }
  EEPROM.put(0, sThreshold); 
  
  Serial.println("Calibración OK. ¡Arrancando en 1 segundo!");
  delay(1000); 
}

// =================================================================
// EL CEREBRO FUZZY (La Lógica Difusa)
// =================================================================
void calcularFuzzyPID(float errorAbsoluto) {
  // Nuestro error máximo es 250. Dividimos en 3 zonas:
  
  if (errorAbsoluto < 60) { 
    // ZONA VERDE (Recta o ligerísima desviación) -> Modo Suave
    Kp_actual = Kp_base * 0.7;  
    Kd_actual = Kd_base * 0.5;  
  } 
  else if (errorAbsoluto >= 60 && errorAbsoluto < 150) {
    // ZONA AMARILLA (Curva normal) -> Modo Alerta
    Kp_actual = Kp_base * 1.2;  
    Kd_actual = Kd_base * 1.5;  
  } 
  else {
    // ZONA ROJA (Peligro, casi perdemos la línea) -> Modo Volantazo
    Kp_actual = Kp_base * 2.5;  
    Kd_actual = Kd_base * 2.0;  
  }
}


void setup() {
  init_GPIO();
  Serial.begin(9600);
  Serial.println("Iniciando sistema...");

  if (digitalRead(PIN_BOTON) == LOW) {
    Serial.println("Jumper detectado. Calibrando...");
    calibrarSensoresAuto(); 
  } else {
    Serial.println("Modo Carrera. Recuperando memoria de EEPROM...");
    EEPROM.get(0, sThreshold);
    Serial.println("¡Memoria cargada. A CORRER!");
  }
}

void loop() {
  tracking();
}

/* ============ Lógica de Tracking ============ */
int calcularError(int s0, int s1, int s2, int s3, int s4, int suma) {
  // Si perdemos la línea (suma == 0), tiramos del recuerdo para girar al máximo
  if (suma == 0) return (lastError > 0) ? 250 : -250;
  
  // Cálculo matemático del centro de masas de la línea
  return (s0 * -250 + s1 * -100 + s2 * 0 + s3 * 100 + s4 * 250) / suma;
}

void tracking() {
  // OJO: Asumimos que línea negra da un valor > sThreshold. 
  int s0 = (analogRead(sensor1) > sThreshold[0]) ? 1 : 0;
  int s1 = (analogRead(sensor2) > sThreshold[1]) ? 1 : 0;
  int s2 = (analogRead(sensor3) > sThreshold[2]) ? 1 : 0;
  int s3 = (analogRead(sensor4) > sThreshold[3]) ? 1 : 0;
  int s4 = (analogRead(sensor5) > sThreshold[4]) ? 1 : 0;

  int suma = s0 + s1 + s2 + s3 + s4;
  float error = calcularError(s0, s1, s2, s3, s4, suma);
  float absError = abs(error);

  // Reducimos la velocidad base en las curvas cerradas
  int baseSpeed;
  if (absError < 50) baseSpeed = BASE_SPEED;                              
  else baseSpeed = constrain(map(absError, 50, 250, BASE_SPEED, MIN_SPEED), MIN_SPEED, BASE_SPEED);

  // --- LLAMAMOS AL CEREBRO FUZZY ---
  calcularFuzzyPID(absError);

  // Cálculo PD estándar usando las constantes mutantes del Fuzzy
  P = error; 
  D = error - lastError; 
  lastError = error;
  
  filteredD = D_FILTER * D + (1.0 - D_FILTER) * filteredD;
  
  // ¡Magia aplicada!
  float correction = (Kp_actual * P) + (Kd_actual * filteredD);
  
  setMotors(baseSpeed + correction, baseSpeed - correction);
}