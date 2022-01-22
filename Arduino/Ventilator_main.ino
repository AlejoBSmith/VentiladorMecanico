#include <StringSplitter.h>
#include <WTimer.h>
WTimer mytimers;
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MedianFilterLib2.h>
#include <movingAvg.h>
#include <Average.h>
MedianFilter2<int> medianFilter2(5);

#include "Variables.h"
Adafruit_ADS1115 ads1115;
#define sfm3300i2c 0x40
#include <Separador.h>
Separador HMI;
bool SensorDetected,FlowSens,PressSens,DiffSens;
String StringEntrada;
String bit_inicio,CaractValve,Modo,Ent_Valve,Ent_PEED_D,Ent_PS,Ent_Vesp,Ent_TP,Ent_Te;

//Promediados de VIF, VEF, PIP, y Presion para modoAsisitido:
movingAvg VIF_promedio(5);
movingAvg VEF_promedio(5);
movingAvg PIP_promedio(5);
movingAvg promediomovil(5);

//Sensirion SDP811
#include <Arduino.h>
#include <SensirionI2CSdp.h>
SensirionI2CSdp sdp;


void setup() 
{
    // Configuración de salidas y seteo de frecuencia a 700Hz
    pinMode(Val_Ins,OUTPUT);
    pinMode(Val_Exh,OUTPUT);
    analogWriteFrequency(36,Frecuencia);
    analogWriteResolution(Resolucion);
    //Iniciar Promediados 
    promediomovil.begin();
    VIF_promedio.begin();
    VEF_promedio.begin();
    PIP_promedio.begin();
    
    //--------------------------------------------------------------
    Serial.begin(115200); // Velocidad del Puerto Serial
    ads1115.begin(0x48); // Inicia comunicación I2C con el ADS1115
    // Establecer comunicación con el SFM3300
    Wire.begin(); // Iniciar comunicación I2C
    while(!Serial) {} // let serial console settle
    Wire.beginTransmission(sfm3300i2c);
    Wire.write(0x10); // start continuous measurement
    Wire.write(0x00); // command 0x1000
    Wire.endTransmission();
    //--------------------------------------------------------------
    //Sensirion SDP811
    sdp.begin(Wire, SDP8XX_I2C_ADDRESS_1);
    sdp.stopContinuousMeasurement();
    error_sdp = sdp.readProductIdentifier(productNumber, serialNumber, serialNumberSize);
    error_sdp = sdp.startContinuousMeasurementWithDiffPressureTCompAndAveraging();
    //mytimers.presetTimer(0,5000,TON); // Preset the timer #1 for x seconds (3000 ms), type TON
    //mytimers.presetTimer(1,5000,TON);
    //mytimers.initTimers(millis());
    Serial.flush();
}
void loop() 
{

    if (Serial.available() > 0) {
    StringEntrada = Serial.readStringUntil('\n');

    StringSplitter *splitter = new StringSplitter(StringEntrada, ',', 15);  // new StringSplitter(string_to_split, delimiter, limit)
    inicio = splitter->getItemAtIndex(0).toInt();
    Porcentaje_apertura_valvula = splitter->getItemAtIndex(1).toInt();
    presion_soporte = splitter->getItemAtIndex(2).toInt();
    Peep_deseado = splitter->getItemAtIndex(3).toInt();
    Volumen_deseado = splitter->getItemAtIndex(4).toInt();
    Tiempo_Expiracion_ms = splitter->getItemAtIndex(5).toInt();
    Tiempo_Plateau_ms = splitter->getItemAtIndex(6).toInt();
    inicio_modoAsistido=splitter->getItemAtIndex(7).toInt();
    Start_caract=splitter->getItemAtIndex(8).toInt();
    TipoCaract_Rampa=splitter->getItemAtIndex(9).toInt();
    TipoCaract_Escalon=splitter->getItemAtIndex(10).toInt();
   // Tiempo_incremento_rampa=splitter->getItemAtIndex(11).toInt();
   // incremento_n_rampa=splitter->getItemAtIndex(12).toInt();
    
    }
    
    mytimers.runTimers(millis());
    // Declaracion de variable de tipo struct 
    MTIntegrator Integrator_Flujo,Integrator_DeltaP;
    FilterMovingAverage FilterMovAvg_Flujo,FilterMovAvg_DeltaP;
    //Proporcional P;
    //------------------------------------------------------------------------------
    LeerADS1115();
    LeerSFM3300();
    LeerSDP811();
    //------------------------------------------------------------------------------
    Calculos(FilterMovAvg_DeltaP, Integrator_DeltaP, FilterMovAvg_Flujo,Integrator_Flujo); //Se calcula la presion, deltaP,Volumen,etc
    Statics_Vol_Max(volumen_inspiracion);
    Statics_Vol_Min(volumen_inspiracion);
    Statics_Presion_Max(Presion);
    Mostrar_Datos();
    modoAsistido(Presion);
    //Calculo del tiempo de ciclo de respiracion
    tiempofinal_ciclo=tiempoinicial_ciclo;
    tiempoinicial_ciclo=millis();
    deltaT_ciclo=tiempoinicial_ciclo-tiempofinal_ciclo;

    if (deltaT_ciclo <= 50)
    {
    acc_ciclo=acc_ciclo+deltaT_ciclo;
    } 

    
    switch (estado)
    {
//-------------------------------------------------------
      case 0: //Modo de espera "Standby"
        
        caso = 0;
        CierreValvulaInspiracion();
        AperturaValvulaExhalacion();
        EtapaResp = 0;
        acc_ciclo=0;
        VIF=0;
        //VEF=10;
        PIP=0;
        
        if (inicio == 1 && Start_caract == 0 && Zero_calibration == 0) // Si se presiona el boton On/Off en el HMI
        {
           Start_caract = false;
           Zero_calibration = false;
           estado = 1;          
        }
        if(Start_caract== 1 && inicio == 0 && Zero_calibration == 0) // Si se presiona el botón en el HMI
        {
           n = valor_inicial_PWM;
           estado = 10;
        }
        if (Zero_calibration == 1 && inicio == 0 && Start_caract == 0) // Si se presiona el botón en el HMI
        {
           f = 1;
           estado = 11;   
        }      
      break; // Cierre del case 0
//-------------------------------------------------------        
      case 1: // Cuando está presionado el botón de start en el HMI
        AuxEtapa = false;
        caso = 1;
        if (Caract_Pulmon == 1)
        {
          if(AuxCaractPulmon >= valor_final_PWM)
          {
            AuxCaractPulmon = valor_inicial_PWM;
            inicio = 0;
          }
          else
          AuxCaractPulmon = AuxCaractPulmon + 50;
        }
        tiempo_ciclo_actual= millis();
        estado = 2;
        integral=0;
        volumen_inspiracion=0;
       
      break;
//-------------------------------------------------------
      case 2: //Comienzo inhalacion       
        EtapaResp = 1;
        caso = 2;
        CierreValvulaExhalacion();
        volumen_inspiracion = Integral_Flujo(Integrator_Flujo,Flujo);     
        if (ModoOperacion == 1) // Modo Volumen Control
        {
          if(Presion >= (Peep_deseado + presion_soporte)||(Volumen_deseado <=  volumen_inspiracion))
          {             
            // Cierre válvula de Inspiración
            CierreValvulaInspiracion();
            estado = 3;  
          }
        else //Apertura valvula de inhalación
          {
            EstadoValvulaInspiracion = true;
            if(Caract_Pulmon == 1)
            {
               analogWrite(Val_Ins,AuxCaractPulmon);         
            }
            else
            {
               AperturaValvulaInspiracion();
            }
          }
        }
      
      break;
//-------------------------------------------------------
      case 3: //Tiempo de Plateau
      integral=0;
      caso = 3;
        EtapaResp = 2;
        if (mytimers.timer[0].Done)
        {
          mytimers.timer[0].Start = false;
          Pplat = Presion;
          
          time_out = false;
          estado = 4;
          Tiempo_Inspiracion = (millis()- tiempo_ciclo_actual)/1000;
          AperturaValvulaExhalacion();          
        }
        else
        {
          
          mytimers.presetTimer(0,(Tiempo_Plateau_ms),TON);
          mytimers.timer[0].Start = true;     
        }
           
      break;
//-------------------------------------------------------
      case 4: //Exhalacion     
      caso = 4;
       integral=0;
         //VEF=VIF/2;
         if (mytimers.timer[0].Done)
         {
           CierreValvulaExhalacion();
           mytimers.timer[0].Start = false;
           Peep = Presion;
           if (inicio==1){
           estado = 2;   // Si lo manda al estado 0, abre exhalación!
           }
           else{
             estado=0;
           }
           
           EtapaResp =3 ;  
           duracion_ciclo=acc_ciclo; //en ms  
           acc_ciclo=0; //reinicia el contador del tiempo para los BPM
             
         }
         else
         {
           mytimers.presetTimer(0,(Tiempo_Expiracion_ms),TON);
           if (inicio_modoAsistido ==1)
           {
             if (mytimers.timer[0].Counter >= (Tiempo_Expiracion_ms/3)) //Modo asistido, monitorea umbral al 33% del tiempo de espiración
              {
               
               if (derivada>=150) //Umbral establecido 
               {
                estado = 2;
                derivada=0;
                duracion_ciclo=acc_ciclo;
                acc_ciclo=0;
                mytimers.timer[0].Start = false;
                break;
               }
              }
           }
           mytimers.timer[0].Start = true;  
           if(Presion <= Peep_deseado)   
           {
             EtapaResp = 4;
             AuxEtapa = true;
             CierreValvulaExhalacion();       
           }
           else
           {
             if(AuxEtapa == false)
             EtapaResp = 5;           
           }
         }
                       
      break;

      case 10:
      caso = 10;
      CaracterizarValvula(Tiempo_incremento_rampa,incremento_n_rampa);
      break;

      default:
        //Nada de momento
      break;
} // Fin del swtich
} //Fin void loop()
void LeerADS1115()
{
  SensorDetected=ads1115.begin(0x48);
  if(SensorDetected){
  sensor_presion_bits = ads1115.readADC_SingleEnded(1); // Sensor de Presión
  PressSens=true;
  }
  else{
  PressSens=false;
  sensor_presion_bits = 0;
  }
}
void LeerSFM3300()
{
  if (2 == Wire.requestFrom(sfm3300i2c, 2))
    { 
        // just keep reading SLM (Standard Liter per Minute)
        uint16_t a = Wire.read(); // only two bytes need to be read
        uint8_t  b = Wire.read(); // if we don't care about CRC
        a = (a<<8) | b;
        sensor_flujo_bits = ((float)a - 32000) / 140;
    }
}
void LeerSDP811()
{
   error_sdp = sdp.readMeasurement(differentialPressure, temperature);
}
void CaracterizarValvula(int Tiempo_incremento_rampa,int incremento_n_rampa)
{

    tiempofinal_caract=tiempoinicial_caract;
    tiempoinicial_caract=millis();
    deltaT_caract=tiempoinicial_caract-tiempofinal_caract;
    
    if (deltaT_caract <= 50)
    {
    acc_caract=acc_caract+deltaT_caract;
    } 

    if (TipoCaract_Rampa == 0 && Start_caract == 1 && TipoCaract_Escalon == 0) 
  {
    acc_caract=0; //Tiempo transcurrido inicializado
    subirPwm = 1;
    n=Pwm_Min;
    analogWrite(Val_Ins,n);
    digitalWrite(Val_Exh,LOW);
  }

    if (TipoCaract_Rampa == 0 && Start_caract == 0 && TipoCaract_Escalon == 0)
    {
      estado=0;
      acc_caract=0;
      
    }
  //Serial.println("Caracterizar Valvula");
  if(TipoCaract_Rampa == 1 && Start_caract == 1 && TipoCaract_Escalon == 0)
{ // 1
    if((n<=Pwm_Max) && subirPwm == 1)
    { // 2
      analogWrite(Val_Ins,n);
      digitalWrite(Val_Exh,LOW);
      mytimers.presetTimer(1,(Tiempo_incremento_rampa),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
        { // 3
       
          mytimers.timer[1].Start = false;
          n = n + incremento_n_rampa;
    
          if(n>= Pwm_Max)
            { // 4
              bajarPwm = 1;
              subirPwm = 0;
            } // 4
        } // 3
    } // 2
    else if((n>=Pwm_Min) && bajarPwm == 1)
    { // 5
      analogWrite(Val_Ins,n);
      digitalWrite(Val_Exh,LOW);
      mytimers.presetTimer(1,(Tiempo_incremento_rampa),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
          { // 6
         
          mytimers.timer[1].Start = false;
          n = n - incremento_n_rampa;
    
            if(n <= Pwm_Min )
                { // 7
                bajarPwm = 0;
                subirPwm = 1;
                } // 7
            } // 6
    } // 5
} // 1 

if(TipoCaract_Escalon == 1 && Start_caract == 1 && TipoCaract_Rampa==0)
{ // 1
    if((n<=Pwm_Max) && subirPwm == 1)
    { // 2
      analogWrite(Val_Ins,Pwm_Max);
      mytimers.presetTimer(1,(250),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
        { // 3
         
          mytimers.timer[1].Start = false;
          n = Pwm_Max;
          
          if(n>= Pwm_Max)
            { // 4
              bajarPwm = 1;
              subirPwm = 0;
            } // 4
        } // 3
    } // 2
    else if((n>=Pwm_Min) && bajarPwm == 1)
    { // 5
      analogWrite(Val_Ins,Pwm_Min);
      digitalWrite(Val_Exh,HIGH);
      mytimers.presetTimer(1,(7000),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
          { // 6
         
          mytimers.timer[1].Start = false;
          n = Pwm_Min;
      
            if(n <= Pwm_Min )
                { // 7
                bajarPwm = 0;
                subirPwm = 1;
                } // 7
            } // 6
    } // 5
} // 1 



} // Final de la funcion

void modoAsistido(float presion_entrada)
{
  mediana=medianFilter2.AddValue(presion_entrada);
  promedioant = promedio;
  promedio = promediomovil.reading(mediana);
  derivada = (promedio-promedioant)/0.005;
}
void Calculos(struct FilterMovingAverage FilterMovAvg_DeltaP, struct MTIntegrator Integrator_DeltaP,struct FilterMovingAverage FilterMovAvg_Flujo, struct MTIntegrator Integrator_Flujo)
{
    // Calculo de datos respiratorios
    IE = Tiempo_Expiracion_ms/(Tiempo_Inspiracion+Tiempo_Plateau_ms); //en ms
   // duracion_ciclo = tiempo_ciclo_actual - tiempo_ciclo_anterior;
    //tiempoaccum = tiempoaccum + int(duracion_ciclo);
    if(duracion_ciclo != 0)
        BPM_calculado = 60000/duracion_ciclo;
    else
    {
        //tiempo_ciclo_anterior = tiempo_ciclo_actual;
        MV = (volumen_inspiracion*BPM_calculado)/1000;
    }
    // Cálculo de presiones
    //Presion = (sensor_presion_bits-presion_zero_cal)/factor_sensor_presion-3.9220;
    Presion= ((0.002131)*sensor_presion_bits) -3.565;  //
    Flujo=  ((0.6545)*sensor_flujo_bits);
    
    //Serial.print("Presion:");
    //Serial.println(Presion);
	  // Cálculo del flujo diferencial y con sensirion
	  DeltaP = (sensor_diferencial_bits-diferencial_zero_cal)/factor_sensor_diferencial; //Diferencial
    
    DeltaP_Avg = MTFilterMovingAverage_DeltaP(FilterMovAvg_DeltaP,DeltaP); //Primero filtramos el valor diferencial
    volumen_diferencial = Integral_DeltaP(Integrator_DeltaP, DeltaP_Avg); // Luego calculamos el volumen diferencial
    Flujo_Avg = MTFilterMovingAverage_Flujo(FilterMovAvg_Flujo,sensor_presion_bits); //Primero filtramos el valor del sensor de flujo


    //Ec

    
}

// Programación de funciones básicas
void CierreValvulaInspiracion()
{
    EstadoValvulaInspiracion = false;
    analogWrite(Val_Ins,Pwm_Min); // Cierre de válvula de inspiración, por qué no cero?
}

void CierreValvulaExhalacion()
{
    EstadoValvulaExhalacion = false;
    digitalWrite(Val_Exh,LOW); // Cierre de válvula de Exhalación 
}

void AperturaValvulaExhalacion()
{
    EstadoValvulaExhalacion = true;
    
	  digitalWrite(Val_Exh,HIGH); // Abre válvula de Exhalación
}

void AperturaValvulaInspiracion()
{
    EstadoValvulaInspiracion = true;
    PwmOut = ((valor_inicial_PWM+(valor_final_PWM-valor_inicial_PWM))*Porcentaje_apertura_valvula/100);
	  analogWrite(Val_Ins,PwmOut); // Abre válvula de Inspiración
    
}

float DataMean(float sensor)
{
    Average <float> ave(100); // Buffer Mean 
    for (int i = 0; i < 100; i++)
    {
        ave.push(sensor);
        Mean = (ave.mean()); //Mean
    }
    return Mean;
}

void ZeroCalibration()
{
    if(f<=datos_zero_cal)
    {
        // Usa la función MTBasicsMean para encontrar el promedio
        if(Zero_calibration)
        {
            datos_presion_zero_cal = DataMean(Presion);
            datos_flujo_zero_cal = DataMean(Flujo);
            datos_diferencial_zero_cal = DataMean(sensor_diferencial_bits);
            f =f+1;
        }
        else
        {
            presion_zero_cal = datos_presion_zero_cal;
            flujo_zero_cal = datos_flujo_zero_cal;
            diferencial_zero_cal = datos_diferencial_zero_cal;
            //Reset
            datos_presion_zero_cal = 0;
            datos_flujo_zero_cal = 0;
            datos_diferencial_zero_cal = 0;

            Zero_calibration = false;
		    f=1;
        }
    }
}


void Mostrar_Datos()
{
        currenttimeM=millis();
        tiempomuestra=currenttimeM-lasttimeM;
        Serial.print("Presion: ");
        Serial.print(Presion);
        Serial.print(",");
        Serial.print("Peep: ");
        Serial.print(Peep);
        Serial.print(",");
        Serial.print("Presion_soporte: ");
        Serial.print(presion_soporte);
        Serial.print(",");
        Serial.print("Flujo: ");
        Serial.print(Flujo);
        Serial.print(",");
        Serial.print("Volumen deseado: ");
        Serial.print(Volumen_deseado);  //Ingresado por usuario
        Serial.print(", ");
        Serial.print("Volumen inspiracion: ");
        Serial.print(volumen_inspiracion);  //Calculado
        Serial.print(",");
        Serial.print("PIP: ");
        Serial.print(PIP);
        Serial.print(",");
        Serial.print("VEF: ");
        Serial.print(VEF);
        Serial.print(",");
        Serial.print("VIF: ");
        Serial.print(VIF);
        Serial.print(",");
        Serial.print("T_muestreo: ");
        Serial.print(tiempomuestra);
        Serial.print(", ");
        Serial.print("PWM Out: ");
        Serial.print(PwmOut);
        Serial.print(", ");
        Serial.print("Timer_Count_0: ");
        Serial.print(mytimers.timer[0].Counter);
        Serial.print(", ");
        Serial.print("Etapa_Res: ");
        Serial.print(EtapaResp);
        Serial.print(", ");
        Serial.print("Derivada_Presion: ");
        Serial.print(derivada);
        Serial.print(", ");
        Serial.print("duracion_ciclo: ");
        Serial.print(duracion_ciclo);
        Serial.print(", ");
        Serial.print("BPM: ");
        Serial.print(BPM_calculado);
        Serial.print(", ");
        Serial.print("PWM_Caract: ");
        Serial.print(n);
        Serial.print(", ");
        Serial.print("T. Trans_Caract: ");
        Serial.print(acc_caract);
        Serial.print(", ");
        Serial.print("MV: ");
        Serial.print(MV);
        Serial.print(", ");
        Serial.print("Diferencial_Presion: ");
        Serial.print(differentialPressure);
        Serial.print(", ");
        Serial.print("Caso: ");
        Serial.print(caso);
        Serial.print(", ");
        Serial.print("String: ");
        Serial.print(StringEntrada);
        Serial.println();
        lasttimeM=currenttimeM;
       
}

float Statics_Vol_Max(float volumen_inspiracion)
{
     VIF_promedio_cal=VIF_promedio.reading(volumen_inspiracion);
     if (VIF<= VIF_promedio_cal)
     {
      VIF=VIF_promedio_cal;
     }
     
    return VIF;
}

float Statics_Vol_Min(float volumen_inspiracion)
{
   
   VEF_promedio_cal=VEF_promedio.reading(volumen_inspiracion);
     if (VEF>= VEF_promedio_cal && VEF_promedio_cal > 0)
     {
      VEF=VEF_promedio_cal;
     }
     
    return VEF;
}

float Statics_Presion_Max(float Presion)
{
   PIP_promedio_cal=PIP_promedio.reading(Presion);
     if (PIP<= PIP_promedio_cal)
     {
      PIP=PIP_promedio_cal;
     }
     
    return PIP;
}
// Programación de funciones especiales
float Integral_Flujo(struct MTIntegrator Integrator_Flujo, float entrada)
   {
    //Asignación de valores a variables del struct
    Integrator_Flujo.Enable = 1; Integrator_Flujo.Gain = 1000;
    Integrator_Flujo.Update = true; Integrator_Flujo.In = entrada;
    Integrator_Flujo.OutPresetValue = 0; Integrator_Flujo.SetOut = true;
    Integrator_Flujo.HoldOut = false;

    //Variables internas de la función
    unsigned long Ultima_Muestra = 0, Ultima_Impresion = 0, dt=0;
    
 
  
    const float conversion = 5.0 / 1024 * 1e-3;
    
    // Calculo dt viejo unsigned long  dt = millis();
    // Calculo dt nuevo:
    lasttime=currenttime;
    currenttime=millis();
    dt=currenttime-lasttime;
 
   

    // Cálculo de la integral
    integral += Integrator_Flujo.In * (dt);
    //Ultima_Muestra = elapseddt;
    Integrator_Flujo.Out = Integrator_Flujo.Gain * (integral*conversion);
    
    return Integrator_Flujo.Out;
   }

float Integral_DeltaP(struct MTIntegrator Integrator_DeltaP, float entrada)
   {
    //Asignación de valores a variables del struct
    Integrator_DeltaP.Enable = 1; Integrator_DeltaP.Gain = 15.68;
    Integrator_DeltaP.Update = true; Integrator_DeltaP.In = entrada;
    Integrator_DeltaP.OutPresetValue = 0; Integrator_DeltaP.SetOut = true;
    Integrator_DeltaP.HoldOut = false;

    //Variables internas de la función
    unsigned long Ultima_Muestra = 0;
    unsigned long Ultima_Impresion = 0;
    float integral = 0;
    const float conversion = 5.0 / 1024 * 1e-3;
    unsigned long dt = millis();

    // Cálculo de la integral
    integral += Integrator_DeltaP.In * (dt - Ultima_Muestra);
    Ultima_Muestra = dt;
    Integrator_DeltaP.Out = Integrator_DeltaP.Gain * (integral*conversion);
    return Integrator_DeltaP.Out;
   }

float MTFilterMovingAverage_Flujo(struct FilterMovingAverage FilterMovAvg_Flujo, float entrada)
{
    //Asignación de valores a variables del struct
    FilterMovAvg_Flujo.Enable = 1;
    FilterMovAvg_Flujo.WindowLength = 5;
    FilterMovAvg_Flujo.Update = true;
    FilterMovAvg_Flujo.In = entrada;

    //Parametros
    int readings [FilterMovAvg_Flujo.WindowLength];
    int readIndex  = 0;
    long total  = 0;
    
    //Aplicación de fórmulas
    long average; //Perform average on sensor readings
    total = total - readings[readIndex]; // subtract the last reading:
    readings[readIndex] = FilterMovAvg_Flujo.In; // read the sensor:
    total = total + readings[readIndex]; // add value to total:
    readIndex = readIndex + 1;// handle index
    if (readIndex >= FilterMovAvg_Flujo.WindowLength)
        readIndex = 0;
    FilterMovAvg_Flujo.Out = total / FilterMovAvg_Flujo.WindowLength; // calculate the average:
    return FilterMovAvg_Flujo.Out;
}

float MTFilterMovingAverage_DeltaP(struct FilterMovingAverage FilterMovAvg_DeltaP, float entrada)
{
    //Asignación de valores a variables del struct
    FilterMovAvg_DeltaP.Enable = 1;
    FilterMovAvg_DeltaP.WindowLength = 5;
    FilterMovAvg_DeltaP.Update = true;
    FilterMovAvg_DeltaP.In = entrada;

    //Parametros
    int readings [FilterMovAvg_DeltaP.WindowLength];
    int readIndex  = 0;
    long total  = 0;
    
    //Aplicación de fórmulas
    long average; //Perform average on sensor readings
    total = total - readings[readIndex]; // subtract the last reading:
    readings[readIndex] = FilterMovAvg_DeltaP.In; // read the sensor:
    total = total + readings[readIndex]; // add value to total:
    readIndex = readIndex + 1;// handle index
    if (readIndex >= FilterMovAvg_DeltaP.WindowLength)
        readIndex = 0;
    FilterMovAvg_DeltaP.Out = total / FilterMovAvg_DeltaP.WindowLength; // calculate the average:
    return FilterMovAvg_DeltaP.Out;
}


float Controlador_P(struct Proporcional P, unsigned int Peep_deseado, unsigned int presion_soporte,unsigned int presion)
{
    //Asignacion de Variables
    P.pasado = 0;
    P.Tiempo_Muestreo = 1;
    P.SetValue =  Peep_deseado + presion_soporte;
    P.ActValue = presion;
    P.Kp = 30;

    P.ahora = millis(); //Tiempo Actual
    P.dt = P.ahora - P.pasado; // Diferencial de tiempo
    if(P.dt >= P.Tiempo_Muestreo) //Si se supera el tiempo de muestreo
    {
        P.error = P.SetValue - P.SetValue; //Error - Feedback
        P.Out = P.Kp*P.error; //Señal de Control
        P.pasado = P.ahora; //Actualización del tiempo
    }
    return P.Out;
}