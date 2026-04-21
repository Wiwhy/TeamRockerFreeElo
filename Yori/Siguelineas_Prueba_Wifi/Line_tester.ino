#include "WiFiEsp.h"
#include "WiFiEspUdp.h"

// --- CONFIGURACIÓN WI-FI ---
char ssid[] = "ROBOT_COMPETICION"; 
char pass[] = "12345678"; 
int status = WL_IDLE_STATUS;
unsigned int localPort = 8080;
WiFiEspUDP Udp;

// ⚠️ MODO DEL ROBOT (0 = Frenado, 1 = Corriendo, 2 = Girando Derecha, 3 = Adelante)
int modo_robot = 0;
unsigned long ultimo_wifi = 0;   

#define MAX_SPEED  255
int velocidad_base = 210; // Velocidad en rectas perfectas (ahora es variable)
#define MIN_SPEED  0      // Velocidad base en giro extremo

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

// --- PINES DE SENSORES ---
#define sensor1 A4 
#define sensor2 A3 
#define sensor3 A2 
#define sensor4 A1 
#define sensor5 A0 

// ================= VALORES PID DIRECTOS =================
float P_verde = 0.2;  float D_verde = 5.0;
float P_ama = 0.4;    float D_ama = 10.0;
float P_rojo = 1.0;   float D_rojo = 15.0;

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

void stop_bot() {
  analogWrite(speedPinLB, 0); analogWrite(speedPinRB, 0); analogWrite(speedPinL, 0); analogWrite(speedPinR, 0);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, LOW);
  digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, LOW);
  digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, LOW);
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

/* ============ Control Avanzado ============ */
void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed  = constrain(leftSpeed,  -MAX_SPEED, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, -MAX_SPEED, MAX_SPEED);
  
  if (leftSpeed >= 0) { 
    FL_fwd(leftSpeed); RL_fwd(leftSpeed);
  } else { 
    FL_bck(-leftSpeed); RL_bck(-leftSpeed); 
  }
  
  if (rightSpeed >= 0) { 
    FR_fwd(rightSpeed); RR_fwd(rightSpeed); 
  } else { 
    FR_bck(-rightSpeed); RR_bck(-rightSpeed); 
  }
}

void setup() {
  init_GPIO();
  Serial.begin(115200);   
  Serial1.begin(115200);  

  Serial.println("Iniciando Wi-Fi...");
  WiFi.init(&Serial1);
  WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK);
  Udp.begin(localPort);
  
  Serial.println("¡Modo DIOS listo! Red: ROBOT_COMPETICION");
  Serial.println("Robot frenado. Usa la App para reanudar.");
}

void loop() {
  // 1. Revisar mensajes Wi-Fi cada 50ms
  if (millis() - ultimo_wifi > 50) {
    revisarTelemetria();
    ultimo_wifi = millis();
  }

  // 2. Moverse, Frenar o Girar manualmente
  if (modo_robot == 1) {
    tracking();
  } 
  else if (modo_robot == 2) {
    setMotors(120, -120); // Gira a la derecha sobre sí mismo
  } 
  else if (modo_robot == 3) {
    setMotors(velocidad_base, velocidad_base); // SOLO ADELANTE a la velocidad configurada
  }
  else {
    stop_bot();
  }
}

// ==========================================================
// EL RECEPTOR DE TELEMETRÍA (Modo Dios)
// ==========================================================
void revisarTelemetria() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    char packetBuffer[128]; 
    int len = Udp.read(packetBuffer, 127);
    if (len > 0) {
      packetBuffer[len] = '\0';
      if (packetBuffer[0] == 'F') {
        char *token = strtok(packetBuffer, ",");
        if (token != NULL) {
          token = strtok(NULL, ","); if(token) velocidad_base = atoi(token);
          token = strtok(NULL, ","); if(token) P_verde = atof(token);
          token = strtok(NULL, ","); if(token) D_verde = atof(token);
          token = strtok(NULL, ","); if(token) P_ama = atof(token);
          token = strtok(NULL, ","); if(token) D_ama = atof(token);
          token = strtok(NULL, ","); if(token) P_rojo = atof(token);
          token = strtok(NULL, ","); if(token) D_rojo = atof(token);
          Serial.println("¡Velocidad y PID Actualizados!");
        }
      } 
      else if (packetBuffer[0] == 'S') { 
        modo_robot = 0;
        Serial.println("¡EMERGENCIA! Robot parado.");
      } 
      else if (packetBuffer[0] == 'R') { 
        modo_robot = 1;
        Serial.println("Reanudando marcha...");
      }
      else if (packetBuffer[0] == 'D') { 
        modo_robot = 2;
        Serial.println("¡Girando a la derecha desde boxes!");
      }
      else if (packetBuffer[0] == 'A') { 
        modo_robot = 3;
        Serial.println("¡Avanzando recto (Prueba de motores)!");
      }
    }
  }
}

/* ============ EL CEREBRO FUZZY ============ */
void calcularFuzzyPID(float errorAbsoluto) {
  if (errorAbsoluto < 60) { 
    Kp_actual = P_verde;
    Kd_actual = D_verde;  
  } 
  else if (errorAbsoluto >= 60 && errorAbsoluto < 150) {
    Kp_actual = P_ama;
    Kd_actual = D_ama;  
  } 
  else {
    Kp_actual = P_rojo;
    Kd_actual = D_rojo;  
  }
}

/* ============ Lógica de Tracking ============ */
int calcularError(int s0, int s1, int s2, int s3, int s4, int suma) {
  if (suma == 0) return (lastError > 0) ? 250 : -250;
  return (s0 * -250 + s1 * -100 + s2 * 0 + s3 * 100 + s4 * 250) / suma;
}

void tracking() {
  // Lectura Digital Directa
  int s0 = !digitalRead(sensor1);
  int s1 = !digitalRead(sensor2);
  int s2 = !digitalRead(sensor3);
  int s3 = !digitalRead(sensor4);
  int s4 = !digitalRead(sensor5);
  int suma = s0 + s1 + s2 + s3 + s4;
  
  float error = calcularError(s0, s1, s2, s3, s4, suma);
  float absError = abs(error);

  int baseSpeed;
  if (absError < 20) {
    baseSpeed = velocidad_base; // Usa la velocidad configurada
  } else {
    // Escala la velocidad: frena en giros extremos para dar tiempo al PID
    baseSpeed = map(absError, 20, 250, velocidad_base, MIN_SPEED);
    baseSpeed = constrain(baseSpeed, MIN_SPEED, velocidad_base);
  }

  calcularFuzzyPID(absError);

  P = error; 
  D = error - lastError; 
  lastError = error;
  filteredD = D_FILTER * D + (1.0 - D_FILTER) * filteredD;
  
  float correction = (Kp_actual * P) + (Kd_actual * filteredD);
  
  int leftSpeed  = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;
  
  setMotors(leftSpeed, rightSpeed);
}