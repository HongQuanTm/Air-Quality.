#include<WiFi.h>
#include<Firebase_ESP_Client.h>
#include<addons/TokenHelper.h>
#include <DS18B20.h>
#include<SimpleDHT.h>
#include <Arduino.h>
#include<string.h>
 
#define WIFI_SSID "khong vao dc dau"
#define WIFI_PASSWORD "hoangdeptrai"
#define DATABASE_URL "https://bkfet-cf67d-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyAo_P16eNg36uR-w8ZK516Kjf0BARjrxfw"
#define FIREBASE_PROJECT_ID "bkfet-cf67d"
#define USER_EMAIL "nguyengiakham2002@gmail.com"
#define USER_PASSWORD "khamnguyen"

 
// Sensor DS18B20 
//cam bien nay dung de do gia tri nhiet do
float tempC;  // bien gia tri nhiet do
DS18B20 ds(15);   //ket noi cam bien DS18B20 vơi chan GPIO 15
float getTemperature()
    {
    tempC=ds.getTempC(); // ham lay gia tri nhiet do theo thang celcius
    Serial.print("Temp: ");
    Serial.print(tempC);
    Serial.print(" C \n");
    return tempC;     // tra ve gia tri nhiet do
    }



// Sensor DHT22
//cam bien nay dung de do gia tri do am
int pinDHT22 = 18;  // ket noi cam bien DHT22 voi chan GPIO 18
SimpleDHT22 dht22(pinDHT22);
byte temperature,humidity;
int err;
float getHumidity() {        // ham lay gia tri do am
    err = SimpleDHTErrSuccess;
    if ((err = dht22.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)  //ham kiem tra xem cam bien DHT22 co bi loi hay khong
    {
        Serial.print("Read DHT22 failed, err="); Serial.print(SimpleDHTErrCode(err));
        Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    }
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" % ");
    return humidity;  // tra ve gia tri do am
}




// Sensor DSM501A
// cam bien nay dung de do mat do tap trung cua hat bui pm2.5 va pm10
int pin = 5;  //DSM501A input D8
unsigned long duration; // tong thoi gian gia tri dien ap tai Vout2 la LOW =0.7V
unsigned long starttime; // thoi diem bat dau do
unsigned long endtime;  // thoi diem ket thuc do
unsigned long sampletime_ms = 10000; //thoi gian lay mau
unsigned long lowpulseoccupancy = 0; // LPO time
float ratio;  // ti so LPO time/ thoi gian lay mau
int concentration =0;   // mat do hat bui
int getDustInfo(){
  duration = pulseIn(pin, LOW);   // thoi gian gia tri low trong khoang thoi gian lay mau
  lowpulseoccupancy += duration;   
  endtime = millis();
  if ((endtime-starttime) > sampletime_ms)
    { 
    ratio = lowpulseoccupancy/(sampletime_ms*10.0); // Integer percentage 0=>100  // gia tri LPO time ratio
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // dua tren bai bao
    Serial.print("lowpulseoccupancy: ");
    Serial.print(lowpulseoccupancy);
    Serial.print(" ratio: ");   // ti so LPO time/ thoi gian lay mau
    Serial.print(ratio);
    Serial.print(" concentration: ");
    Serial.println(concentration);   // gia tri mat do hat bui, don vi pcs/0.01cf
    starttime=millis();
    lowpulseoccupancy=0; 
  }
  else {
    Serial.println(" Calculating dust concentration...( 30s/ time ) ");
  }
  return concentration; // tra ve gia tri nong do bui PM2.5 PM10
}



// Sensor Mq135
//cam bien nay dung de do nong do khi CO2
int mq=34, mqRead;  // ket noi cam bien MQ135 vao chan GPIO 34
int getCo2(){      // ham lay gia tri nong do khi CO2
  mqRead = analogRead(mq);    // doc gia tri analog tu chan 34
  mqRead = map(mqRead,200,3026,150,600);  // anh xa gia tri analog doc duoc len khoang gia tri tu 150 den 600, day la gia tri nong do CO2
  Serial.print("Co2: ");
  Serial.print(mqRead);
  Serial.println(" PPM ");
  return mqRead; // tra ve gia tri nong do CO2, don vi PPM
}

// cap nhat du lieu len firestore
String documentPath = "DataAir/xilTeBsQhvyLGBtfvACn"; // gan duong dan document cua firestore


FirebaseData fbdo; //tao bien data
FirebaseAuth auth; // tao bien xac thuc
FirebaseConfig config; // tao bien cau hinh
FirebaseJson content; // tao bien noi dung

void firestoreDataUpdate(float temp, float dust, int co2,int hum)
{
  if(WiFi.status() == WL_CONNECTED && Firebase.ready()){

    content.clear();  // xoa cac gia tri cua bien content 
    content.set("fields/CO2/stringValue", String(co2).c_str());  // dat gia tri co2 vao trong fields/CO2/stringValue cua content
    content.set("fields/bui/stringValue",String( dust).c_str());  // dat gia tri dust vao trong fields/bui/stringValue cua content
    content.set("fields/nhietdo/stringValue", String(temp).c_str());  // dat gia tri temp vao trong fields/nhietdo/stringValue cua content
    content.set("fields/doam/stringValue", String(hum).c_str());  // dat gia tri hum vao trong fields/doam/stringValue cua content
  
    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "CO2,bui,nhietdo,doam")) // cap nhat cac thong tin bien content len firestore
    {
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str()); // hien thi noi dung va trang thai cap nhat
        return;
    }
    else
    {
        Serial.println(fbdo.errorReason()); // neu khong cap nhat duoc hien thi loi
    }
    }
}



float dust,temp,hum; // khoi tao cac bien nong do bui PM2.5 PM10, nhiet do, do am
int Co2; // khoi tao bien nong do CO2
void setup()
{
    Serial.begin(115200); // dat gia tri toc do baud cho man hinh Serial
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // ket noi wifi
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)  //kiem tra tinh trang ket noi wifi
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());  // hien thi IP cua thiet bi khi ket noi wifi
    Serial.println();
    
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);  // hien thi phien ban firebase client
    
    config.api_key = API_KEY;  //gan gia trị API KEY
    auth.user.email = USER_EMAIL;  // gan gia tri ten email cua chủ project firebase
    auth.user.password = USER_PASSWORD;  //gan gia tri mat khau cua email tren
    
    config.token_status_callback = tokenStatusCallback; // ham callback goi gia tri trang thai cua token
    
    // Limit the size of response payload to be collected in FirebaseData
    Firebase.begin(&config, &auth); // ket noi voi firebase
    Firebase.reconnectWiFi(true);  //tu dong ket noi voi mang wifi  

    starttime=millis();

 }

void loop()
{
    
    Serial.println("Cac gia tri");
    // lay gia tri cho tung bien
    temp= getTemperature(); // nhiet do
    dust= getDustInfo(); // mat do bui PM2.5, PM10
    hum= getHumidity(); // do am
    Co2= getCo2(); // nong do CO2
    delay(2000);
    Serial.println("Waiting...");

    if( Firebase.ready() ) 
    {
        firestoreDataUpdate(temp,dust,Co2,hum);  // cap nhat 4 du lieu len firestore
        Serial.print("Saved on firestore \n");
    }
}

