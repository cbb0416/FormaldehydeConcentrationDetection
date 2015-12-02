#include "LiquidCrystal.h"
#include "mf_aqm_ch2o.h"
#include "TimerOne.h"
#include "SoftwareSerial.h"
#include <stdlib.h>
#include "stdio.h"
#include "string.h"

/*管脚配置*/
//LCD 1602,KEY board: AD0,D4,D5,D6,D7,D8,D9,D10
//甲醛传感器(SoftwareSerial): D12:Rx, D13:Tx
//GSM board(SoftwareSerial): D2:Rx, D3:Tx, D11:Reset

LiquidCrystal lcd(8, 9, 4, 5, 6, 7); 
mf_aqm_ch2o ch2o;
//SoftwareSerial gsm(2,3);
#define gsm Serial
#define HWSTART_PIN 11

float ch2o_concentration;

void SIM900A_StartHwReset( void )
{
    digitalWrite(HWSTART_PIN, HIGH);
    //Serial.println("Start HW RESET");
}

void SIM900A_StopHwReset( void )
{
    digitalWrite(HWSTART_PIN, LOW);  
    //Serial.println("Stop HW RESET");
}

void SIM900A_SendATCPOWD( void )
{
     gsm.println("AT+CPOWD=1");
     //Serial.println("AT+CPOWD=1");
}

void SIM900A_SendATCGATT( void )
{
    gsm.println("AT+CGATT=1");
    //Serial.println("AT+CGATT=1");  
}

void SIM900A_SendATCIPMODE( void )
{
    gsm.println("AT+CIPMODE=1");  
    //Serial.println("AT+CIPMODE=1");
}

void SIM900A_SendATCIPSTART( void )
{
    gsm.println("AT+CIPSTART=\"TCP\",\"api.wsncloud.com\",\"80\"");  
    //Serial.println("AT+CIPSTART");  
}

void SIM900A_SendPost( void )
{
    char buffer[16];
    u16 temp;
    ch2o_concentration = ch2o.read_average();
    temp = ch2o_concentration*1000;
    sprintf(buffer, "%d.%03d",temp/1000,temp%1000);
    
    gsm.print("POST /data/v1/numerical/insert?ak=a683525f2703aaa9bdfb6b36f9a9b0b2&id=565f007be4b00415c4386186&value=");
    gsm.print(buffer);     
    gsm.println(" HTTP/1.1");
    gsm.println("Host: api.wsncloud.com");
    gsm.println(); 
    //Serial.println("Send Post");  
}

void SIM900A_SendDataToServer( void )
{
    static u8 step = 100;
    static u16 time_sec_count = 0;
    
    switch(step)
    {
        case 0:
            SIM900A_SendATCPOWD();
            time_sec_count = 0;
            step++;
            break;
        case 1:
            if (time_sec_count >= 10)
            {
                SIM900A_StartHwReset();
                time_sec_count = 0;
                step++;
            }
            break;
        case 2:
            if (time_sec_count >= 2)
            {
                SIM900A_StopHwReset();
                time_sec_count = 0;
                step++;
            }
            break;
        case 3:
            if (time_sec_count >= 15)
            {
                SIM900A_SendATCGATT();
                time_sec_count = 0;
                step++;
            }
            break;
        case 4:
            if (time_sec_count >= 10)
            {
                SIM900A_SendATCIPMODE();
                time_sec_count = 0;
                step++;
            }
            break;
        case 5:
            if (time_sec_count >= 10)
            {
                SIM900A_SendATCIPSTART();
                time_sec_count = 0;
                step++;
            }
            break;
        case 6:
            if (time_sec_count >= 10)
            {
                SIM900A_SendPost();
                time_sec_count = 0;
                step++;
            }
            break;
        case 7:
            if (time_sec_count >= 6)
            {
                SIM900A_SendATCPOWD();
                time_sec_count = 0;
                step++;
            }
            break;
        case 100:
            if (time_sec_count >= 300)//5min认为传感器稳定，可上报当前数据
            {
                time_sec_count = 0;
                step = 0;
            }
            break;
        default: 
            if (time_sec_count >= 1737)//每个半小时上报一次数据到传感云网站
            {
                time_sec_count = 0;
                step = 0;
            }
            break;
    }
    time_sec_count++;
}

void setup() {
    u8 i, j;
    gsm.begin(9600);

    /*while (!Serial) {
    ; // wait for Serial port to connect. Needed for Leonardo only
    }*/
    //Serial.println("System Power On!");
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Power On!");
    delay(1000);
    pinMode(HWSTART_PIN, OUTPUT); 
    Timer1.initialize(1000000);
    Timer1.attachInterrupt(SIM900A_SendDataToServer);
    Timer1.start();

    for (i = 0; i < 2; i++)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("System Power On!");  
        lcd.setCursor(0, 1);
        lcd.print("Waiting");
        for ( j = 0; j < 7; j++)
        {
            lcd.setCursor(7+j, 1);
            lcd.print(".");
            delay(999);
        }
    }
}

void loop() {
    static unsigned long disp_refresh_time = millis();
    ch2o.loop();
    if ((ch2o_concentration != ch2o.read_current()) && (millis() - disp_refresh_time) >= 2000)
    {
        u16 temp;
        char buffer[16];

        disp_refresh_time = millis();
        ch2o_concentration = ch2o.read_current();
        
        temp = ch2o_concentration*1000;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("HCHO: ");
        lcd.setCursor(0,1);
        sprintf(buffer, "%d.%03d mg/m3",temp/1000,temp%1000);
        lcd.print(buffer);     
        //Serial.println(buffer);
    }
}
