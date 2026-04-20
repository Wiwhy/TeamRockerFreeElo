#define MAX_SPEED  210
#define BASE_SPEED 190   // Velocidad en rectas perfectas
#define MIN_SPEED  0     // Velocidad base en giro extremo (rotación pura)

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

#define sensor1 A4 // Left most sensor
#define sensor2 A3 // 2nd Left sensor
#define sensor3 A2 // Center sensor
#define sensor4 A1 // 2nd Right sensor
#define sensor5 A0 // Right most sensor

// ================= FUZZY PID =================
// 1. Valores base de tu PID original (Ajusta estos si hace falta)
float Kp_base = 0.2; 
float Kd_base = 5;

// 2. Variables mutantes que cambiará el Fuzzy
float Kp_actual = 0;
float Kd_actual = 0;

float P = 0, D = 0, lastError = 0;
float filteredD = 0;
#define D_FILTER 0.3    // Suavizado derivativo (0.0=máx filtro, 1.0=sin filtro)
// (Integral I_MAX eliminada por estrategia)
// ========================================

/* ============ Motor control ============ */
void FR_fwd(int speed) { digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, HIGH); analogWrite(speedPinR, speed); }
void FR_bck(int speed) { digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW); analogWrite(speedPinR, speed); }
void FL_fwd(int speed) { digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, HIGH); analogWrite(speedPinL, speed); }
void FL_bck(int speed) { digitalWrite(LeftMotorDirPin1, HIGH); digitalWrite(LeftMotorDirPin2, LOW); analogWrite(speedPinL, speed); }
void RR_fwd(int speed) { digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, HIGH); analogWrite(speedPinRB, speed); }
void RR_bck(int speed) { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW); analogWrite(speedPinRB, speed); }
void RL_fwd(int speed) { digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, HIGH); analogWrite(speedPinLB, speed); }
void RL_bck(int speed) { digitalWrite(LeftMotorDirPin1B, HIGH); digitalWrite(LeftMotorDirPin2B, LOW); analogWrite(speedPinLB, speed); }

void stop_bot() {
  analogWrite(speedPinLB, 0); analogWrite(speedPinRB, 0); analogWrite(speedPinL, 0); analogWrite(speedPinR, 0);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, LOW);
  digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, LOW);
  delay(40);
}

void init_GPIO() {
  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT); pinMode(speedPinL, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT); pinMode(speedPinLB, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT); pinMode(speedPinRB, OUTPUT);
  
  pinMode(sensor1, INPUT); pinMode(sensor2, INPUT); pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT); pinMode(sensor5, INPUT);
  stop_bot();
}

void setup() {
  init_GPIO();
  Serial.begin(9600);
}

void loop() {
  tracking();
}

/* ============ EL CEREBRO PUZZY ============ */
void calcularFuzzyPID(float errorAbsoluto) {
  // Nuestro error máximo es 250. Dividimos la reacción en 3 zonas:
  if (errorAbsoluto < 60) { 
    // ZONA VERDE (Recta) -> Modo Suave para no temblar
    Kp_actual = Kp_base * 0.4;  
    Kd_actual = Kd_base * 1;  
  } 
  else if (errorAbsoluto >= 60 && errorAbsoluto < 150) {
    // ZONA AMARILLA (Curva normal) -> Modo Alerta
    Kp_actual = Kp_base * 1;  
    Kd_actual = Kd_base * 2.5 ;  
  } 
  else {
    // ZONA ROJA (Peligro de salida) -> Modo Volantazo
    Kp_actual = Kp_base * 3.2;  
    Kd_actual = Kd_base * 1.7;  
  }
}

/* ============ PID helpers ============ */
int calcularError(int s0, int s1, int s2, int s3, int s4, int suma) {
  if (suma == 0) {
    // Línea perdida → devolver error máximo en la última dirección conocida
    return (lastError > 0) ? 250 : -250;
  }
  return (s0 * -250 + s1 * -100 + s2 * 0 + s3 * 100 + s4 * 250) / suma;
}

void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed  = constrain(leftSpeed,  -MAX_SPEED, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SPEED, MAX_SPEED);
  if (leftSpeed >= 0) { FL_fwd(leftSpeed); RL_fwd(leftSpeed); } else { FL_bck(-leftSpeed); RL_bck(-leftSpeed); }
  if (rightSpeed >= 0) { FR_fwd(rightSpeed); RR_fwd(rightSpeed); } else { FR_bck(-rightSpeed); RR_bck(-rightSpeed); }
}

/* ============ Tracking principal ============ */
void tracking() {
  int s0 = !digitalRead(sensor1);
  int s1 = !digitalRead(sensor2);
  int s2 = !digitalRead(sensor3);
  int s3 = !digitalRead(sensor4);
  int s4 = !digitalRead(sensor5);

  int suma = s0 + s1 + s2 + s3 + s4;
  float error = calcularError(s0, s1, s2, s3, s4, suma);
  float absError = abs(error);

  // ---- Velocidad base variable (Tu lógica original genial) ----
  int baseSpeed;
  if (absError < 20) {
    baseSpeed = BASE_SPEED; // Recta perfecta
  } else {
    baseSpeed = map(absError, 20, 250, BASE_SPEED, MIN_SPEED);
    baseSpeed = constrain(baseSpeed, MIN_SPEED, BASE_SPEED);
  }

  // ---- LLAMAMOS AL CEREBRO PUZZY ----
  calcularFuzzyPID(absError);

  // ---- CÁLCULO PD (Proporcional - Derivativo) ----
  P = error;
  D = error - lastError;
  lastError = error;

  // Filtro low-pass sobre la derivada (suaviza saltos discretos)
  filteredD = D_FILTER * D + (1.0 - D_FILTER) * filteredD;

  // Aplicamos las constantes mutantes (Kp_actual y Kd_actual)
  float correction = (Kp_actual * P) + (Kd_actual * filteredD);

  int leftSpeed  = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;
  
  setMotors(leftSpeed, rightSpeed);
}