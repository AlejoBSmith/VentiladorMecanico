// Declaración de structs
struct MTIntegrator
{
  bool Enable; float Gain;
  bool Update; float In;
  int OutPresetValue; float Out;
  bool SetOut; bool HoldOut;
};

struct FilterMovingAverage
{
  bool Enable; int WindowLength;
  bool Update; float In; float Out;
};

struct Proporcional
{
  unsigned long ahora;
  unsigned long pasado;
  double error;
  double Out;
  float Kp;
  float SetValue;
  float ActValue;
  int dt;
  int Tiempo_Muestreo;

};

// Declaración de variables globales
float Flujo = 0; float Volumen = 0; float DeltaP = 0;
float Flujo_Avg = 0; float DeltaP_Avg = 0; float DeltaP_Final = 0;
float Presion = 0;  // La suma de Peep_deseado + presion_soporte
float DiferencialPresion=0;

//Parametros para ModoAsistido
int inicio_modoAsistido=0;
float mediana;
float promedioant;
float promedio;
float derivada=0;

//Parametros para calcular el tiempo de ciclo 
float tiempofinal_ciclo, tiempoinicial_ciclo, deltaT_ciclo, acc_ciclo, duracion_ciclo;

//Parametros para calcular el tiempo de la caracterizacion
float tiempofinal_caract, tiempoinicial_caract, deltaT_caract, acc_caract, duracion_caract;

//Parametros para calcular VIF, VEF y PIP utilizando promediados.
float VIF_promedio_cal,VEF_promedio_cal,PIP_promedio_cal;

//Parametros para Sensirion SDP811
uint16_t error_sdp;
uint32_t productNumber;
uint8_t serialNumber[8];
uint8_t serialNumberSize = 8;
float differentialPressure;
float temperature;

// Parámetros PWM Teensy 4.1
int Frecuencia = 700; // En Hz
int Pwm_Min = 0; // Valor PWM mínimo permitido
int Pwm_Max = 16383; // Valor PWM máximo permitido
int Resolucion = 14; // bits
int Tiempo = 3000; // ms
int Val_Ins = 36; //Válvula de inspiración
int Val_Exh = 38; //Válvula de exhalación

// Parametros para controlador P desde HMI
unsigned int Peep_deseado = 16;
unsigned int presion_soporte = 20;

//Variables en las cajas de texto el HMI
int estado = 0; //Estado inicial del sistema
int inicio = 0; //Boton On/Off en la HMI
int caso = 0;
int ModoOperacion = 1;
int PorcentajeValvula = 0;
float Tiempo_Inspiracion;
int Tiempo_Plateau_ms = 2000;
int Tiempo_Expiracion_ms=500;
int Volumen_deseado = 500;
int Presion_deseada = 25;

//Parametros de salida Pwm
float valor_final_PWM = 10000; //12500
float valor_inicial_PWM = 4930; //9500
unsigned int Porcentaje_apertura_valvula = 60;
int PwmOut = 0;
int subirPwm = 1;
int bajarPwm = 0;

float aux,ultima ,lasttime=0,currenttime=0,currenttimeM=0,lasttimeM=0,tiempomuestra=0;
float integral = 0;
float dt = 0;

//Variables internas de operación
String bit_inicio;
String CaractValve;
String Modo;
String Ent_Valve;
String Ent_PEED_D;
String Ent_PS;
String Ent_Vesp;
String Ent_TP;
String Ent_Te;
String StringEntrada;

int TipoCaract_Rampa=0;
int TipoCaract_Escalon=0;
int n = Pwm_Min; //Incrementos de pwm para la caracterizacion
int Tiempo_incremento_rampa=1; //ms
int incremento_n_rampa=5;
int TipoCaract = 1;
int Start_caract = 0;
int f;
int AuxCaractPulmon = valor_inicial_PWM;
int datos_zero_cal = 300;

unsigned int EtapaResp;
unsigned int tiempoaccum;

bool Caract_Pulmon = false;
bool AuxEtapa;
bool time_out;
bool EstadoValvulaInspiracion;
bool EstadoValvulaExhalacion;
bool SensorDetected;
bool FlowSens;
bool PressSens;
bool DiffSens;
bool Zero_calibration = false;

float IE;
float tiempo_ciclo_actual = 0;
float tiempo_ciclo_anterior = 0;
float BPM_calculado;
float volumen_inspiracion;
float MV = 0;
float volumen_diferencial;
float presion_zero_cal = 0;
float flujo_zero_cal = 0;
float diferencial_zero_cal = 0;
float Pplat = 0;
float Peep = 0;
float PIP = 0;
float PEF = 0;
float VEF=10;
float VIF = 0;
float Tex = 3;
float sensor_presion_bits = 0;
float sensor_flujo_bits = 0;
float sensor_diferencial_bits = 0;
float datos_presion_zero_cal;
float datos_flujo_zero_cal;
float datos_diferencial_zero_cal;
float factor_sensor_presion = 21.24;
float factor_sensor_flujo = 77;
float factor_sensor_diferencial = 434;
float Mean;