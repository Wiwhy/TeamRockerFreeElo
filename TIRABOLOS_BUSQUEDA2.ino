// Version 2.1
// ============================================================
//  TIRABOLOS - Robot Demoledor de Bolos
//  BUSCAR -> ATACAR -> RETROCEDER (loop infinito)
//
//  REESCRITURA COMPLETA v2.1:
//  - Logica de estados simplificada al maximo.
//  - medirDistancia() usa el metodo estandar del HC-SR04 (probado).
//    Timeout 25 ms (rango hasta ~4 m) para nunca perder ecos cercanos.
//    Formula clasica: cm = duracion_us / 58.
//  - Sin checks ni hacks en el ultrasonido: trigger limpio y pulseIn directo.
//  - Delays entre estados configurables en la seccion de arriba.
//  - Secuencia al detectar linea:
//      1. Freno de inercia (retroceso breve)
//      2. Pausa quieto
//      3. Retroceso al centro
//      4. Pausa quieto
//      5. Vuelta a BUSCAR
// ============================================================

// ----------------------------------------------------------
//  AJUSTAR AQUI
// ----------------------------------------------------------

const int VEL_BUSQUEDA  = 55;   // PWM giro de busqueda (0-255)
const int VEL_ATAQUE    = 160;  // PWM avance de ataque  (0-255)
const int VEL_RETROCESO = 160;  // PWM marcha atras      (0-255)

const int DIST_DETECCION    = 90;   // cm: detecta objetivo a esta distancia
const int TIEMPO_GIRO       = 60;   // ms girando entre cada disparo US
const int TIEMPO_RETROCESO  = 500;  // ms de marcha atras hacia el centro

// Delays entre estados (robot completamente quieto)
const int VEL_FRENO         = 120;  // PWM del freno de inercia
const int TIEMPO_FRENO      = 80;   // ms de retroceso para absorber inercia
const int DELAY_POST_LINEA  = 1000; // ms quieto tras detectar linea
const int DELAY_POST_RETRO  = 1000; // ms quieto antes de volver a buscar

// ----------------------------------------------------------
//  PINES MOTORES (L298N)
// ----------------------------------------------------------
#define speedPinR           9
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1   26
#define LeftMotorDirPin2   28
#define speedPinL          10

#define speedPinRB         11
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B  6
#define LeftMotorDirPin1B   7
#define LeftMotorDirPin2B   8
#define speedPinLB         12

// ----------------------------------------------------------
//  PINES ULTRASONIDO
// ----------------------------------------------------------
#define TRIG  30
#define ECHO  31

// ----------------------------------------------------------
//  PINES SENSORES DE LINEA
// ----------------------------------------------------------
#define S1  A4
#define S2  A3
#define S3  A2
#define S4  A1
#define S5  A0

// ----------------------------------------------------------
//  ESTADOS
// ----------------------------------------------------------
enum Estado { BUSCAR, ATACAR, RETROCEDER };
Estado        estado     = BUSCAR;
unsigned long tRetroIni  = 0;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(RightMotorDirPin1,  OUTPUT);
  pinMode(RightMotorDirPin2,  OUTPUT);
  pinMode(LeftMotorDirPin1,   OUTPUT);
  pinMode(LeftMotorDirPin2,   OUTPUT);
  pinMode(speedPinR,          OUTPUT);
  pinMode(speedPinL,          OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT);
  pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(LeftMotorDirPin1B,  OUTPUT);
  pinMode(LeftMotorDirPin2B,  OUTPUT);
  pinMode(speedPinRB,         OUTPUT);
  pinMode(speedPinLB,         OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  parar();
  Serial.println(F("=== TIRABOLOS v2.1 ==="));
}

// ============================================================
//  LOOP
// ============================================================
void loop() {

  bool linea = hayLinea();

  switch (estado) {

    // --------------------------------------------------------
    case BUSCAR:
      if (linea) {
        Serial.println(F("[BUSCAR] linea -> freno + retro"));
        frenarYPausar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }
      // Girar
      velocidad(VEL_BUSQUEDA);
      girar();
      delay(TIEMPO_GIRO);

      // Medir
      {
        int d = medir();
        Serial.print(F("US: ")); Serial.print(d); Serial.println(F(" cm"));
        if (d > 0 && d <= DIST_DETECCION) {
          estado = ATACAR;
          Serial.println(F("[BUSCAR] -> ATACAR"));
        }
      }
      break;

    // --------------------------------------------------------
    case ATACAR:
      if (linea) {
        Serial.println(F("[ATACAR] linea -> freno + retro"));
        frenarYPausar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }
      velocidad(VEL_ATAQUE);
      avanzar();
      break;

    // --------------------------------------------------------
    case RETROCEDER:
      if (millis() - tRetroIni < (unsigned long)TIEMPO_RETROCESO) {
        velocidad(VEL_RETROCESO);
        retroceder();
      } else {
        parar();
        Serial.print(F("[RETRO] centro. Pausa "));
        Serial.print(DELAY_POST_RETRO);
        Serial.println(F(" ms"));
        delay(DELAY_POST_RETRO);
        estado = BUSCAR;
        Serial.println(F("[RETRO] -> BUSCAR"));
      }
      break;
  }
}

// ============================================================
//  medir()
//  Metodo estandar HC-SR04. Timeout 25ms (rango ~4m).
//  Formula: cm = us / 58  (identica a cm = us * 0.01724)
//  Sin hacks, sin checks extra: trigger limpio y pulseIn directo.
// ============================================================
int medir() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 25000UL); // 25 ms timeout
  if (us == 0) return 999;
  return (int)(us / 58);
}

// ============================================================
//  hayLinea()  — LOW = negro
// ============================================================
bool hayLinea() {
  return (!digitalRead(S1) || !digitalRead(S2) || !digitalRead(S3) ||
          !digitalRead(S4) || !digitalRead(S5));
}

// ============================================================
//  frenarYPausar()
//  1. Retroceso breve (freno de inercia)
//  2. Pausa quieto
// ============================================================
void frenarYPausar() {
  Serial.print(F("  freno ")); Serial.print(TIEMPO_FRENO); Serial.println(F(" ms"));
  velocidad(VEL_FRENO);
  retroceder();
  delay(TIEMPO_FRENO);

  parar();

  Serial.print(F("  pausa ")); Serial.print(DELAY_POST_LINEA); Serial.println(F(" ms"));
  delay(DELAY_POST_LINEA);
}

// ============================================================
//  MOVIMIENTO
// ============================================================
void velocidad(int v) {
  analogWrite(speedPinL,  v);
  analogWrite(speedPinR,  v);
  analogWrite(speedPinLB, v);
  analogWrite(speedPinRB, v);
}

void avanzar()   { FRf(); FLf(); RRf(); RLf(); }
void retroceder(){ FRb(); FLb(); RRb(); RLb(); }
void girar()     { FRb(); FLf(); RRb(); RLf(); } // giro derecha en sitio

void parar() {
  digitalWrite(RightMotorDirPin1,  LOW); digitalWrite(RightMotorDirPin2,  LOW);
  digitalWrite(LeftMotorDirPin1,   LOW); digitalWrite(LeftMotorDirPin2,   LOW);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B,  LOW); digitalWrite(LeftMotorDirPin2B,  LOW);
  velocidad(0);
}

void FRf() { digitalWrite(RightMotorDirPin1,  LOW);  digitalWrite(RightMotorDirPin2,  HIGH); }
void FRb() { digitalWrite(RightMotorDirPin1,  HIGH); digitalWrite(RightMotorDirPin2,  LOW);  }
void FLf() { digitalWrite(LeftMotorDirPin1,   LOW);  digitalWrite(LeftMotorDirPin2,   HIGH); }
void FLb() { digitalWrite(LeftMotorDirPin1,   HIGH); digitalWrite(LeftMotorDirPin2,   LOW);  }
void RRf() { digitalWrite(RightMotorDirPin1B, LOW);  digitalWrite(RightMotorDirPin2B, HIGH); }
void RRb() { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW);  }
void RLf() { digitalWrite(LeftMotorDirPin1B,  LOW);  digitalWrite(LeftMotorDirPin2B,  HIGH); }
void RLb() { digitalWrite(LeftMotorDirPin1B,  HIGH); digitalWrite(LeftMotorDirPin2B,  LOW);  }
