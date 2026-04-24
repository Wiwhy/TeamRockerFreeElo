// Version 3.2
// ============================================================
//  TIRABOLOS - Robot Demoledor de Bolos
//  BUSCAR -> ATACAR -> RETROCEDER (loop infinito)
//
//  v3.2:
//  - Codigo limpio, organizado por secciones.
//  - Sensores de linea delanteros (S1-S5) y traseros (ST1-ST5).
//  - Maniobra de inicio: retrocede hasta detectar linea trasera,
//    luego vuelve al centro. Activar con INICIO_RETROCESO = true.
//  - Ultrasonido con mediana de 3 lecturas y pausa pre-medicion.
//  - Timeout de busqueda: si no detecta nada, avanza recto.
//  - Todas las velocidades y tiempos son configurables.
// ============================================================


// ************************************************************
//  SECCION 1: CONFIGURACION - AJUSTAR AQUI
// ************************************************************

// --- Maniobra de inicio ---
// Si es true, al encender el robot retrocede hasta detectar
// la linea trasera, luego avanza al centro y empieza a buscar.
const bool INICIO_RETROCESO     = true;
const int  VEL_INICIO_RETRO     = 85;  // PWM marcha atras en maniobra inicio
const int  VEL_INICIO_AVANCE    = 200;  // PWM avance para volver al centro
const int  TIEMPO_INICIO_AVANCE = 560;  // ms avanzando hacia el centro tras detectar linea trasera

// --- Velocidades de movimiento ---
const int VEL_BUSQUEDA          = 160;  // PWM giro de busqueda          (0-255)
const int VEL_ATAQUE            = 200;  // PWM avance de ataque          (0-255)
const int VEL_RETROCESO         = 200;  // PWM marcha atras al centro    (0-255)

// --- Deteccion ultrasonido ---
const int          DIST_DETECCION = 100;     // cm: distancia maxima de deteccion
const int          TIEMPO_GIRO    = 20;      // ms girando entre cada medicion US
const unsigned long US_TIMEOUT    = 6000UL;  // us timeout del sensor (~1m rango)
const int          PAUSA_PRE_MEDIR = 30;     // ms quieto antes de medir (elimina ruido motor)

// --- Retroceso tras detectar linea delantera ---
const int TIEMPO_RETROCESO       = 545;  // ms de marcha atras hacia el centro

// --- Timeout de busqueda ---
// Si pasa este tiempo girando sin detectar nada, avanza recto.
const unsigned long TIMEOUT_BUSQUEDA = 4500UL;  // ms

// --- Delays entre estados ---
const int DELAY_PRE_ATAQUE       = 0;    // ms quieto antes de atacar (tras detectar objetivo)
const int DELAY_POST_RETRO       = 200;  // ms quieto antes de volver a buscar


// ************************************************************
//  SECCION 2: PINES
// ************************************************************

// --- Motores delanteros (L298N) ---
#define speedPinR           9
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1   26
#define LeftMotorDirPin2   28
#define speedPinL          10

// --- Motores traseros (L298N) ---
#define speedPinRB         11
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B  6
#define LeftMotorDirPin1B   7
#define LeftMotorDirPin2B   8
#define speedPinLB         12

// --- Ultrasonido (HC-SR04) ---
#define TRIG  30
#define ECHO  31

// --- Sensores de linea DELANTEROS (borde frontal) ---
#define S1  A4
#define S2  A3
#define S3  A2
#define S4  A1
#define S5  A0

// --- Sensores de linea TRASEROS (borde trasero) ---
#define ST1  A8
#define ST2  A9
#define ST3  A10
#define ST4  A11
#define ST5  A12


// ************************************************************
//  SECCION 3: ESTADOS Y VARIABLES GLOBALES
// ************************************************************

enum Estado { BUSCAR, ATACAR, RETROCEDER };
Estado        estado      = BUSCAR;
unsigned long tRetroIni   = 0;       // timestamp inicio retroceso
unsigned long tBuscarIni  = 0;       // timestamp inicio busqueda


// ************************************************************
//  SECCION 4: SETUP
// ************************************************************

void setup() {
  Serial.begin(9600);

  // --- Configurar pines motores ---
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

  // --- Configurar pines ultrasonido ---
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  // --- Configurar sensores de linea delanteros ---
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  // --- Configurar sensores de linea traseros ---
  pinMode(ST1, INPUT);
  pinMode(ST2, INPUT);
  pinMode(ST3, INPUT);
  pinMode(ST4, INPUT);
  pinMode(ST5, INPUT);

  // --- Estado inicial ---
  parar();
  Serial.println(F("=== TIRABOLOS v3.2 ==="));

  // --- Maniobra de inicio (si esta activada) ---
  if (INICIO_RETROCESO) {
    maniobraInicio();
  }

  // --- Iniciar timer de busqueda ---
  tBuscarIni = millis();
}


// ************************************************************
//  SECCION 5: MANIOBRA DE INICIO
//  Retrocede hasta detectar la linea trasera, luego avanza
//  al centro del ring y empieza el bucle principal.
// ************************************************************

void maniobraInicio() {
  Serial.println(F("[INICIO] Retrocediendo hasta linea trasera..."));

  // Paso 1: Retroceder hasta que los sensores traseros detecten la linea
  velocidad(VEL_INICIO_RETRO);
  retroceder();
  while (!hayLineaTrasera()) {
    // Esperar activamente hasta detectar linea trasera
    delay(5);
  }

  // Paso 2: Parar al detectar la linea
  Serial.println(F("[INICIO] Linea trasera detectada -> parando"));
  parar();
  delay(200);  // pausa breve antes de avanzar

  // Paso 3: Avanzar hacia el centro
  Serial.println(F("[INICIO] Avanzando al centro..."));
  velocidad(VEL_INICIO_AVANCE);
  avanzar();
  delay(TIEMPO_INICIO_AVANCE);

  // Paso 4: Parar en el centro
  parar();
  delay(300);  // pausa antes de empezar a buscar
  Serial.println(F("[INICIO] En el centro. Comenzando busqueda."));
}


// ************************************************************
//  SECCION 6: LOOP PRINCIPAL
// ************************************************************

void loop() {

  // Leer sensores de linea delanteros
  bool linea = hayLinea();

  switch (estado) {

    // --------------------------------------------------------
    //  ESTADO: BUSCAR
    //  Gira buscando un objetivo con el ultrasonido.
    //  Si detecta linea delantera -> frena y retrocede.
    //  Si pasa TIMEOUT_BUSQUEDA sin detectar -> avanza recto.
    // --------------------------------------------------------
    case BUSCAR:

      // Comprobar linea delantera
      if (linea) {
        Serial.println(F("[BUSCAR] linea -> parar + retro"));
        parar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }

      // Timeout: si lleva mucho sin detectar, avanza recto
      if (millis() - tBuscarIni >= TIMEOUT_BUSQUEDA) {
        Serial.println(F("[BUSCAR] timeout -> ATACAR recto"));
        estado = ATACAR;
        break;
      }

      // Girar en busca de objetivo
      velocidad(VEL_BUSQUEDA);
      girar();
      delay(TIEMPO_GIRO);

      // Parar motores y medir con mediana (sin ruido electrico)
      {
        parar();
        delay(PAUSA_PRE_MEDIR);
        int d = medirMediana();
        Serial.print(F("US: ")); Serial.print(d); Serial.println(F(" cm"));

        // Si hay objetivo dentro del rango -> ATACAR
        if (d > 0 && d <= DIST_DETECCION) {
          Serial.println(F("[BUSCAR] -> ATACAR"));
          Serial.print(F("  pausa pre-ataque ")); Serial.print(DELAY_PRE_ATAQUE); Serial.println(F(" ms"));
          delay(DELAY_PRE_ATAQUE);
          estado = ATACAR;
        }
      }
      break;

    // --------------------------------------------------------
    //  ESTADO: ATACAR
    //  Avanza recto hacia el objetivo.
    //  Si detecta linea delantera -> frena y retrocede.
    // --------------------------------------------------------
    case ATACAR:

      // Comprobar linea delantera
      if (linea) {
        Serial.println(F("[ATACAR] linea -> parar + retro"));
        parar();
        estado    = RETROCEDER;
        tRetroIni = millis();
        break;
      }

      // Avanzar a velocidad de ataque
      velocidad(VEL_ATAQUE);
      avanzar();
      break;

    // --------------------------------------------------------
    //  ESTADO: RETROCEDER
    //  Marcha atras hacia el centro durante TIEMPO_RETROCESO ms.
    //  Luego pausa y vuelve a BUSCAR.
    // --------------------------------------------------------
    case RETROCEDER:

      if (millis() - tRetroIni < (unsigned long)TIEMPO_RETROCESO) {
        // Retrocediendo hacia el centro
        velocidad(VEL_RETROCESO);
        retroceder();
      } else {
        // Llegó al centro (o tiempo cumplido)
        parar();
        Serial.print(F("[RETRO] centro. Pausa "));
        Serial.print(DELAY_POST_RETRO);
        Serial.println(F(" ms"));
        delay(DELAY_POST_RETRO);
        estado = BUSCAR;
        tBuscarIni = millis();  // reset timer busqueda
        Serial.println(F("[RETRO] -> BUSCAR"));
      }
      break;
  }
}


// ************************************************************
//  SECCION 7: ULTRASONIDO
// ************************************************************

// --- medir() ---
// Disparo estandar HC-SR04. Devuelve distancia en cm.
// Devuelve 999 si no hay eco o lectura invalida.
int medir() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, US_TIMEOUT);
  if (us == 0) return 999;        // sin eco = nada detectado
  int cm = (int)(us / 58);
  if (cm < 2) return 999;          // < 2cm = ruido, ignorar
  return cm;
}

// --- medirMediana() ---
// Toma 3 lecturas y devuelve la mediana.
// Filtra valores espurios (ruido electrico, ecos falsos).
int medirMediana() {
  int a = medir();
  delayMicroseconds(500);
  int b = medir();
  delayMicroseconds(500);
  int c = medir();
  // Ordenar 3 valores y devolver el del medio
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;  // mediana
}


// ************************************************************
//  SECCION 8: SENSORES DE LINEA
// ************************************************************

// --- hayLinea() ---
// Devuelve true si ALGUN sensor DELANTERO detecta la linea negra.
// Los sensores dan LOW cuando ven negro.
bool hayLinea() {
  return (!digitalRead(S1) || !digitalRead(S2) || !digitalRead(S3) ||
          !digitalRead(S4) || !digitalRead(S5));
}

// --- hayLineaTrasera() ---
// Devuelve true si ALGUN sensor TRASERO detecta la linea negra.
// Misma logica que los delanteros: LOW = negro.
bool hayLineaTrasera() {
  return (!digitalRead(ST1) || !digitalRead(ST2) || !digitalRead(ST3) ||
          !digitalRead(ST4) || !digitalRead(ST5));
}





// ************************************************************
//  SECCION 10: MOVIMIENTO DE MOTORES
// ************************************************************

// --- velocidad() ---
// Establece el PWM de los 4 motores al mismo valor.
void velocidad(int v) {
  analogWrite(speedPinL,  v);
  analogWrite(speedPinR,  v);
  analogWrite(speedPinLB, v);
  analogWrite(speedPinRB, v);
}

// --- Movimientos basicos ---
void avanzar()    { FRf(); FLf(); RRf(); RLf(); }  // todos adelante
void retroceder() { FRb(); FLb(); RRb(); RLb(); }  // todos atras
void girar()      { FRb(); FLf(); RRb(); RLf(); }  // giro derecha en sitio

// --- parar() ---
// Desconecta todos los motores (ambos pines LOW = rueda libre).
void parar() {
  digitalWrite(RightMotorDirPin1,  LOW); digitalWrite(RightMotorDirPin2,  LOW);
  digitalWrite(LeftMotorDirPin1,   LOW); digitalWrite(LeftMotorDirPin2,   LOW);
  digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, LOW);
  digitalWrite(LeftMotorDirPin1B,  LOW); digitalWrite(LeftMotorDirPin2B,  LOW);
  velocidad(0);
}

// --- Funciones individuales de cada motor ---
// FR = Front Right, FL = Front Left, RR = Rear Right, RL = Rear Left
// f = forward, b = backward
void FRf() { digitalWrite(RightMotorDirPin1,  LOW);  digitalWrite(RightMotorDirPin2,  HIGH); }
void FRb() { digitalWrite(RightMotorDirPin1,  HIGH); digitalWrite(RightMotorDirPin2,  LOW);  }
void FLf() { digitalWrite(LeftMotorDirPin1,   LOW);  digitalWrite(LeftMotorDirPin2,   HIGH); }
void FLb() { digitalWrite(LeftMotorDirPin1,   HIGH); digitalWrite(LeftMotorDirPin2,   LOW);  }
void RRf() { digitalWrite(RightMotorDirPin1B, LOW);  digitalWrite(RightMotorDirPin2B, HIGH); }
void RRb() { digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW);  }
void RLf() { digitalWrite(LeftMotorDirPin1B,  LOW);  digitalWrite(LeftMotorDirPin2B,  HIGH); }
void RLb() { digitalWrite(LeftMotorDirPin1B,  HIGH); digitalWrite(LeftMotorDirPin2B,  LOW);  }
