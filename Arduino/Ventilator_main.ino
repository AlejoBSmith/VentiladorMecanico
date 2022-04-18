// Corregir rutina de sdp811, actualmente dura 10 ms

#include <StringSplitter.h>
#include <WTimer.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MedianFilterLib2.h>
#include <movingAvg.h>
#include <Average.h>
#include <Separador.h>
#include "Variables.h"
#include <Arduino.h>
#include <SensirionI2CSdp.h>
#define sfm3300i2c 0x40

MedianFilter2<int> medianFilter2(5);
WTimer mytimers;
Adafruit_ADS1115 ads1115;
Separador HMI;
SensirionI2CSdp sdp;
elapsedMillis PlatTimer;
elapsedMillis ExhTimer;
elapsedMillis PCTimer;
elapsedMillis VCTimerInsp;

//Averaging functions:
movingAvg VIF_promedio(5);
movingAvg VEF_promedio(5);
movingAvg PIP_promedio(5);
movingAvg promediomovil(5);

void setup()
{
  //Set Teensys' IO pins and PWM frequency (PWM frequency is given in valve's datasheets)
  pinMode(Val_Ins,OUTPUT);
  pinMode(Val_Exh,OUTPUT);
  analogWriteFrequency(36,Frecuencia);
  analogWriteResolution(Resolucion);

  //Start averaging functions
  promediomovil.begin();
  VIF_promedio.begin();
  VEF_promedio.begin();
  PIP_promedio.begin();
 
  //--------------------------------------------------------------
  // Starts ADS1115 ADC
  ads1115.begin(0x48); //Address is 0x48 when ADDR pin is connected to GND
  ads1115.setDataRate(RATE_ADS1115_860SPS);
  // Starts communication with Sensirion SFM3300
  Wire.begin(); // Iniciar comunicación I2C
  while(!Serial) {} // let serial console settle
  Wire.beginTransmission(sfm3300i2c);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();
  //Starts communication with Sensirion SDP811
  sdp.begin(Wire, SDP8XX_I2C_ADDRESS_1);
  sdp.stopContinuousMeasurement();
  error_sdp = sdp.readProductIdentifier(productNumber, serialNumber, serialNumberSize);
  error_sdp = sdp.startContinuousMeasurementWithDiffPressureTCompAndAveraging();
  //--------------------------------------------------------------
}

void loop()
{
// First check if any of the operating values from the HMI have changed
  if (Serial.available() > 0)
  {
    StringEntrada = Serial.readStringUntil('\n');
    StringSplitter *splitter = new StringSplitter(StringEntrada, ',', 40);  // new StringSplitter(string_to_split, delimiter, limit)
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
    Tiempo_InspiracionPC_ms=splitter->getItemAtIndex(11).toInt();
    kp=splitter->getItemAtIndex(12).toInt();
    ki=splitter->getItemAtIndex(13).toInt();
    ModoOperacion=splitter->getItemAtIndex(14).toInt();
  }
 
  mytimers.runTimers(millis()); //Timing function

  //------------------------------------------------------------------------------
  // Read sensors
  LeerADS1115();
  LeerSFM3300();
  LeerSDP811();
  //------------------------------------------------------------------------------

  Calculos(); //Sensor data conversion
  Mostrar_Datos(); //Sends variables through serial port
  modoAsistido(Presion);
 
  //Cycle time
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
    case 0: // Idle
      caso = 0;
      CierreValvulaInspiracion(); //Keeps inspiratory valve closed
      AperturaValvulaExhalacion(); //Keeps expiratory valve open
      volumen_inspiracion = 0;
      EtapaResp = 0;
      acc_ciclo = 0;
      VIF = 0;
      PIP = 0;
     
      if (inicio == 1 && Start_caract == 0 && Zero_calibration == 0) // Begins respiratory cycle and forces other operating modes to false
      {
        Start_caract = false;
        Zero_calibration = false;
        offsetFlow = sensor_flujo_bits;
        offsetPressure = ((0.002131)*sensor_presion_bits) -3.565;
        offsetDiffPress = sensor_diferencial_bits/37960.3;
        estado = 1;
      }
      if(Start_caract== 1 && inicio == 0 && Zero_calibration == 0) // Begins system characterization and forces other operating modes to false
      {
        n = valor_inicial_PWM;
        estado = 10;
      }
      if (Zero_calibration == 1 && inicio == 0 && Start_caract == 0) // Sets offset values for sensor readings
      {
        f = 1;
        estado = 11;  
      }      
    break;
  //-------------------------------------------------------        
    case 1:
      AuxEtapa = false;
      caso = 1;
      if (Caract_Pulmon == 1) //Varies PWM output only if lung characterization is chosen
      {
        if(AuxCaractPulmon >= valor_final_PWM)
        {
          AuxCaractPulmon = valor_inicial_PWM;
          inicio = 0;
        }
        else
        {
        AuxCaractPulmon = AuxCaractPulmon + 50;
        }
      }
      tiempo_ciclo_actual= millis();
      PCTimer = 0; //only for first breath
      VCTimerInsp = 0;
      estado = 2;
      integral=0;
      volumen_inspiracion=0; //Reset inspiratory volume to avoid integral drift
    break;
  //-------------------------------------------------------
    case 2: //Inspiration begin    
      EtapaResp = 1;
      caso = 2;
      CierreValvulaExhalacion();
      volumen_inspiracion += Flujo*deltaT_ciclo/60;

      if (ModoOperacion == 1) // Volume control mode
      {
        if(Presion >= (Peep_deseado + presion_soporte)||(Volumen_deseado <=  volumen_inspiracion)||(VCTimerInsp>Tiempo_max_insp_VC))
        {            
          // Closes inspiration valve if pressure or volume are exceeded
          CierreValvulaInspiracion();
          PIP = Presion;
          PlatTimer = 0;
          estado = 3; //State change
        }
        else // Opens inspiratory valve if neither set pressure or volume have been reached
        {
          EstadoValvulaInspiracion = true;
          if(Caract_Pulmon == 1) //For lung characterization, variable pwm
          {
            analogWrite(Val_Ins,AuxCaractPulmon);
          }
          else
          {
            AperturaValvulaInspiracion();
          }
        }
      }
      if (ModoOperacion == 0) //Pressure control mode
      {
        if ((PCTimer >= Tiempo_InspiracionPC_ms)||(Volumen_deseado <=  volumen_inspiracion))
        {
          // Maintains desired pressure until preset time is reached or volume is exceeded
          CierreValvulaInspiracion();
          PIP = Presion;
          errorIntegral = 0;
          PlatTimer = 0;
          estado = 3; //State change
        }
        else
        {
          EstadoValvulaInspiracion = true;
          AperturaValvulaInspiracion();
        }
      }
    break;
  //-------------------------------------------------------
    case 3: //Plateau (hold) time
      integral=0; //Stops integrating flow
      caso = 3;
      EtapaResp = 2;
      if (ModoOperacion == 1)
      {
        if ((PlatTimer >= Tiempo_Plateau_ms))
        {
          Pplat = Presion;
          ExhTimer = 0;
          Tiempo_Inspiracion = (millis()- tiempo_ciclo_actual)/1000; // Inspiration time including hold calc
          estado = 4;
          AperturaValvulaExhalacion();          
        }
      }
      else
      {
        Pplat = Presion;
        ExhTimer = 0;
        Tiempo_Inspiracion = (millis()- tiempo_ciclo_actual)/1000; // Inspiration time including hold calc
        estado = 4;
        AperturaValvulaExhalacion();
      }
    break;
  //-------------------------------------------------------
    case 4: //Exhalacion    
      caso = 4;
      integral=0;
      if (ExhTimer >= Tiempo_Expiracion_ms)
      {
        Peep = Presion;
        if (inicio==1)
        {
          PCTimer = 0;
          VCTimerInsp = 0;
          estado = 2;
          volumen_inspiracion = 0;
          PwmOutAux = valor_inicial_PWM;
        }
        else
        {
          estado=0;
        }
        EtapaResp =3 ;  
        duracion_ciclo=acc_ciclo; //en ms  
        acc_ciclo=0; //reinicia el contador del tiempo para los BPM
      }
      else
      {
        if (inicio_modoAsistido ==1) //If assisted mode is on
        {
          if (ExhTimer >= (Tiempo_Expiracion_ms/3)) //Checks wether 33% of expiratory time has passed before checking pressure variability threshold
          {
            if (derivada>=150) //Pressure sensitivity (diff Pressure)
            {
              PCTimer = 0;
              estado = 2; //Starts a new inpiratory cycle
              duracion_ciclo=acc_ciclo;
              acc_ciclo=0;
              break;
            }
          }
        }
        if(Presion <= Peep_deseado)   //PEEP setting
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
  //-------------------------------------------------------
    case 10: //Valve characterization
    caso = 10;
    CaracterizarValvula(Tiempo_incremento_rampa,incremento_n_rampa);
    break;
  //-------------------------------------------------------
    default:
    //Nada de momento
    break;
  }
}

void LeerADS1115()
{
  sensor_presion_bits = ads1115.readADC_SingleEnded(1); // Sensor de Presión
}

void LeerSFM3300()
{
  if (2 == Wire.requestFrom(sfm3300i2c, 2))
  {
    uint16_t a = Wire.read();
    uint8_t  b = Wire.read();
    a = (a<<8) | b;
    sensor_flujo_bits = ((float)a - 32000) / 140;
    if(abs(sensor_flujo_bits)>=100)
    {
      Wire.begin(); // Iniciar comunicación I2C
      while(!Serial) {} // let serial console settle
      Wire.beginTransmission(sfm3300i2c);
      Wire.write(0x10);
      Wire.write(0x00);
      Wire.endTransmission();
    }
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
  if(TipoCaract_Rampa == 0 && Start_caract == 0 && TipoCaract_Escalon == 0)
  {
    estado=0;
    acc_caract=0;
  }
  if(TipoCaract_Rampa == 1 && Start_caract == 1 && TipoCaract_Escalon == 0)
  {
    if((n<=Pwm_Max) && subirPwm == 1)
    {
      analogWrite(Val_Ins,n);
      digitalWrite(Val_Exh,LOW);
      mytimers.presetTimer(1,(Tiempo_incremento_rampa),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
      {
        mytimers.timer[1].Start = false;
        n = n + incremento_n_rampa;
        if(n>= Pwm_Max)
        {
          bajarPwm = 1;
          subirPwm = 0;
        }
      }
    }
    else if((n>=Pwm_Min) && bajarPwm == 1)
    {
      analogWrite(Val_Ins,n);
      digitalWrite(Val_Exh,LOW);
      mytimers.presetTimer(1,(Tiempo_incremento_rampa),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
      {
        mytimers.timer[1].Start = false;
        n = n - incremento_n_rampa;
        if(n <= Pwm_Min )
        {
          bajarPwm = 0;
          subirPwm = 1;
        }
      }
    }
  }

if(TipoCaract_Escalon == 1 && Start_caract == 1 && TipoCaract_Rampa==0)
  {
    if((n<=Pwm_Max) && subirPwm == 1)
    {
      analogWrite(Val_Ins,Pwm_Max);
      mytimers.presetTimer(1,(250),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
      {
        mytimers.timer[1].Start = false;
        n = Pwm_Max;
        if(n>= Pwm_Max)
        {
          bajarPwm = 1;
          subirPwm = 0;
        }
      }
    }
    else if((n>=Pwm_Min) && bajarPwm == 1)
    {
      analogWrite(Val_Ins,Pwm_Min);
      digitalWrite(Val_Exh,HIGH);
      mytimers.presetTimer(1,(7000),TON); // Se coloca un PT de 100ms
      mytimers.timer[1].Start = true; // Iniciamos el timer
      if(mytimers.timer[1].Done) // Si el timer termina, entonces...
      {
        mytimers.timer[1].Start = false;
        n = Pwm_Min;
        if(n <= Pwm_Min )
        {
          bajarPwm = 0;
          subirPwm = 1;
        }
      }
    }
  }
}

void modoAsistido(float presion_entrada)
{
  mediana=medianFilter2.AddValue(presion_entrada);
  promedioant = promedio;
  promedio = promediomovil.reading(mediana);
  derivada = (promedio-promedioant)/0.005;
}

void Calculos()
{
  // Respiratory variable calcs
  IE = Tiempo_Expiracion_ms/(Tiempo_Inspiracion+Tiempo_Plateau_ms); //en ms
  MV = (volumen_inspiracion*BPM_calculado)/1000;
  if(duracion_ciclo != 0)
  {
    BPM_calculado = 60000/duracion_ciclo;
  }
  // Variable calc
  Presion = ((0.002131)*sensor_presion_bits) -3.565-offsetPressure;
  Flujo = sensor_flujo_bits-offsetFlow; //Scale factor from sfm3000 datasheet
  DeltaP = sensor_diferencial_bits/37960.3-offsetDiffPress; //Scale factor from sdp811 datasheet
}

void CierreValvulaInspiracion()
{
    EstadoValvulaInspiracion = false;
    analogWrite(Val_Ins,Pwm_Min);

    analogWrite(Val_PC,Pwm_Min);
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
  if (ModoOperacion == 1)
  {
  EstadoValvulaInspiracion = true;
  PwmOut = ((valor_inicial_PWM+(valor_final_PWM-valor_inicial_PWM))*Porcentaje_apertura_valvula/100);
  PwmOut = constrain(PwmOut,Pwm_Min,Pwm_Max);
  analogWrite(Val_Ins,PwmOut); // Abre válvula de Inspiración
  }
  else
  {
    errorPC = presion_soporte+Peep_deseado-Presion;
    errorIntegral += errorPC*(deltaT_ciclo/100);
    PwmOut = (kp*errorPC+ki*errorIntegral)+valor_inicial_PWM;
    PwmOut = constrain(PwmOut,valor_inicial_PWM,valor_final_PWM);
    PwmOutPC = 9400;
    PwmOutPC = constrain(PwmOutPC,valor_inicial_PWM,valor_final_PWM);
    analogWrite(Val_PC,PwmOutPC);
    analogWrite(Val_Ins,PwmOut); // Abre válvula de Inspiración
  }
}

void Mostrar_Datos()
{
  Serial.print("PresionM: ");
  Serial.print(Presion);
  Serial.print(",");
  Serial.print("PeepM: ");
  Serial.print(Peep);
  Serial.print(",");
  Serial.print("PresionD: ");
  Serial.print(presion_soporte);
  Serial.print(",");
  Serial.print("FlujoM: ");
  Serial.print(Flujo);
  Serial.print(",");
  Serial.print("VolumenM: ");
  Serial.print(volumen_inspiracion);
  Serial.print(",");
  Serial.print("PIPM: ");
  Serial.print(PIP);
  Serial.print(",");
  Serial.print("T_muestreoM: ");
  Serial.print(deltaT_ciclo);
  Serial.print(", ");
  Serial.print("PWMOut: ");
  Serial.print(PwmOutAux);
  Serial.print(", ");
  Serial.print("ErrorPC: ");
  Serial.print(errorPC);
  Serial.print(", ");
  Serial.print("Etapa_Res: ");
  Serial.print(EtapaResp);
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
  Serial.print("MV: ");
  Serial.print(MV);
  Serial.print(", ");
  Serial.print("DiffPressM: ");
  Serial.print(differentialPressure);
  Serial.print(", ");
  Serial.print("String: ");
  Serial.print(StringEntrada);
  Serial.println();
}
