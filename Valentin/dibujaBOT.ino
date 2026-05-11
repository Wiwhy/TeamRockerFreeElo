// Pines de los motores (L298N)
#define speedPinR 9
#define RightMotorDirPin1  22
#define RightMotorDirPin2  24
#define LeftMotorDirPin1  26
#define LeftMotorDirPin2  28
#define speedPinL 10

#define speedPinRB 11
#define RightMotorDirPin1B  5
#define RightMotorDirPin2B 6
#define LeftMotorDirPin1B 7
#define LeftMotorDirPin2B 8
#define speedPinLB 12

// ========================================================
// ======== CONFIGURACIÓN PRINCIPAL DE FIGURAS ============
// ========================================================

// 1. SELECTOR DE FIGURA (1 = Triángulo | 2 = Cuadrado | 3 = Rectángulo)
int figura_a_dibujar = 3; 

// 2. VELOCIDADES INDEPENDIENTES POR LÍNEA (0 a 255)
// Si el robot tiene menos fuerza hacia atrás o hacia un lado, sube su velocidad aquí.

// --- Velocidades TRIÁNGULO ---
int vel_tri_adelante   = 100;
int vel_tri_diag_der   = 100;
int vel_tri_diag_izq   = 100;

// --- Velocidades CUADRADO ---
int vel_cuad_adelante  = 75;
int vel_cuad_derecha   = 75; // Strafe
int vel_cuad_atras     = 85; // Ejemplo: un poco más alta si le cuesta ir hacia atrás
int vel_cuad_izquierda = 75; // Strafe

// --- Velocidades RECTÁNGULO ---
int vel_rect_adelante  = 75;
int vel_rect_derecha   = 75;
int vel_rect_atras     = 85; 
int vel_rect_izquierda = 75;


// 3. Pausa en las esquinas (En milisegundos. 500 = medio segundo)
int tiempo_espera_esquinas = 500; 


// ========================================================
// ============= MEDIDAS INDEPENDIENTES (CM) ==============
// ========================================================
// Al tener velocidades distintas, es posible que necesites 
// pedirle más o menos CM a un lado específico para que cierre bien.

// --- TRIÁNGULO ---
float tri_adelante   = 25.0; 
float tri_diag_der   = 35.0; 
float tri_diag_izq   = 35.0; 

// --- CUADRADO (Objetivo: 20x20 cm) ---
float cuad_adelante  = 20.0; 
float cuad_derecha   = 20.0; 
float cuad_atras     = 20.0; 
float cuad_izquierda = 20.0; 

// --- RECTÁNGULO (Objetivo: 28x12 cm) ---
float rect_adelante  = 32.0; 
float rect_derecha   = 22.0; 
float rect_atras     = 27.0; 
float rect_izquierda = 22.0; 

// ========================================================
// ================= CALIBRACIÓN FÍSICA ===================
// ========================================================

// --- Calibración TRIÁNGULO ---
float ms_cm_frente_triangulo = 21.0; 
float ms_cm_diagonal         = 35.0; 
float compensacion_angulo_tri = 0.60; 

// --- Calibración CUADRADO y RECTÁNGULO ---
float ms_cm_frente_cuad_rect = 28.0; 
float ms_cm_lateral          = 43.3; 

// ========================================================

void setup() {
  pinMode(RightMotorDirPin1, OUTPUT); pinMode(RightMotorDirPin2, OUTPUT);
  pinMode(LeftMotorDirPin1, OUTPUT); pinMode(LeftMotorDirPin2, OUTPUT);
  pinMode(RightMotorDirPin1B, OUTPUT); pinMode(RightMotorDirPin2B, OUTPUT);
  pinMode(LeftMotorDirPin1B, OUTPUT); pinMode(LeftMotorDirPin2B, OUTPUT);

  pinMode(speedPinL, OUTPUT); pinMode(speedPinR, OUTPUT);
  pinMode(speedPinLB, OUTPUT); pinMode(speedPinRB, OUTPUT);

  stop_Stop();

  if (figura_a_dibujar == 1) { dibujarTriangulo(); } 
  else if (figura_a_dibujar == 2) { dibujarCuadrado(); } 
  else if (figura_a_dibujar == 3) { dibujarRectangulo(); }
}

void loop() {}

// ==========================================
// RUTINAS DE DIBUJO HOLONÓMICO
// ==========================================

void dibujarTriangulo() {
  // LADO 1: ADELANTE
  int t_lado_1 = tri_adelante * ms_cm_frente_triangulo;
  setMotors(vel_tri_adelante, vel_tri_adelante, vel_tri_adelante, vel_tri_adelante);
  delay(t_lado_1);
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // LADO 2: DIAGONAL ABAJO-DERECHA
  int t_lado_2 = tri_diag_der * ms_cm_diagonal; 
  int v2_fuerte = vel_tri_diag_der;
  int v2_suave = vel_tri_diag_der * compensacion_angulo_tri; 
  setMotors(v2_suave, -v2_fuerte, -v2_fuerte, v2_suave);
  delay(t_lado_2);
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // LADO 3: DIAGONAL ABAJO-IZQUIERDA
  int t_lado_3 = tri_diag_izq * ms_cm_diagonal;
  int v3_fuerte = vel_tri_diag_izq;
  int v3_suave = vel_tri_diag_izq * compensacion_angulo_tri;
  setMotors(-v3_fuerte, v3_suave, v3_suave, -v3_fuerte);
  delay(t_lado_3);
  stop_Stop();
}

void dibujarCuadrado() {
  // 1: ADELANTE
  int t_frente = cuad_adelante * ms_cm_frente_cuad_rect;
  setMotors(vel_cuad_adelante, vel_cuad_adelante, vel_cuad_adelante, vel_cuad_adelante);
  delay(t_frente); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 2: DERECHA (Strafe Right)
  int t_der = cuad_derecha * ms_cm_lateral;
  setMotors(vel_cuad_derecha, -vel_cuad_derecha, -vel_cuad_derecha, vel_cuad_derecha);
  delay(t_der); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 3: ATRÁS
  int t_atras = cuad_atras * ms_cm_frente_cuad_rect;
  setMotors(-vel_cuad_atras, -vel_cuad_atras, -vel_cuad_atras, -vel_cuad_atras);
  delay(t_atras); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 4: IZQUIERDA (Strafe Left)
  int t_izq = cuad_izquierda * ms_cm_lateral;
  setMotors(-vel_cuad_izquierda, vel_cuad_izquierda, vel_cuad_izquierda, -vel_cuad_izquierda);
  delay(t_izq); 
  stop_Stop();
}

void dibujarRectangulo() {
  // 1: ADELANTE
  int t_frente = rect_adelante * ms_cm_frente_cuad_rect;
  setMotors(vel_rect_adelante, vel_rect_adelante, vel_rect_adelante, vel_rect_adelante);
  delay(t_frente); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 2: DERECHA
  int t_der = rect_derecha * ms_cm_lateral;
  setMotors(vel_rect_derecha, -vel_rect_derecha, -vel_rect_derecha, vel_rect_derecha);
  delay(t_der); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 3: ATRÁS
  int t_atras = rect_atras * ms_cm_frente_cuad_rect;
  setMotors(-vel_rect_atras, -vel_rect_atras, -vel_rect_atras, -vel_rect_atras);
  delay(t_atras); 
  if (tiempo_espera_esquinas > 0) { stop_Stop(); delay(tiempo_espera_esquinas); }

  // 4: IZQUIERDA
  int t_izq = rect_izquierda * ms_cm_lateral;
  setMotors(-vel_rect_izquierda, vel_rect_izquierda, vel_rect_izquierda, -vel_rect_izquierda);
  delay(t_izq); 
  stop_Stop();
}

// ==========================================
// CONTROL DE MOTORES UNIFICADO
// ==========================================

void setMotors(int FL, int FR, int RL, int RR) {
  // Front Left
  if (FL >= 0) {
    digitalWrite(LeftMotorDirPin1, LOW); digitalWrite(LeftMotorDirPin2, HIGH); analogWrite(speedPinL, FL);
  } else {
    digitalWrite(LeftMotorDirPin1, HIGH); digitalWrite(LeftMotorDirPin2, LOW); analogWrite(speedPinL, -FL);
  }
  // Front Right
  if (FR >= 0) {
    digitalWrite(RightMotorDirPin1, LOW); digitalWrite(RightMotorDirPin2, HIGH); analogWrite(speedPinR, FR);
  } else {
    digitalWrite(RightMotorDirPin1, HIGH); digitalWrite(RightMotorDirPin2, LOW); analogWrite(speedPinR, -FR);
  }
  // Rear Left
  if (RL >= 0) {
    digitalWrite(LeftMotorDirPin1B, LOW); digitalWrite(LeftMotorDirPin2B, HIGH); analogWrite(speedPinLB, RL);
  } else {
    digitalWrite(LeftMotorDirPin1B, HIGH); digitalWrite(LeftMotorDirPin2B, LOW); analogWrite(speedPinLB, -RL);
  }
  // Rear Right
  if (RR >= 0) {
    digitalWrite(RightMotorDirPin1B, LOW); digitalWrite(RightMotorDirPin2B, HIGH); analogWrite(speedPinRB, RR);
  } else {
    digitalWrite(RightMotorDirPin1B, HIGH); digitalWrite(RightMotorDirPin2B, LOW); analogWrite(speedPinRB, -RR);
  }
}

void stop_Stop() {
  setMotors(0, 0, 0, 0);
}
// Dih
