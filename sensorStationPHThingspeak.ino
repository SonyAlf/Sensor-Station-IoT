//Dikembangkan oleh
//Nama        : Ahmad Sony Alfathani
//Tanggal     : 19 April 2018 (diperbaharui)
//MErupakan perbaharuan dari code http://www.rhelectronics.net/store/radiation-detector-geiger-counter-diy-kit-second-edition.html
//                                https://www.sfe-electronics.com/blog/news/tutorial-pemrograman-dht-sensor-menggunakan-arduino
//Deskripsi program : program ini digunakan untuk Mengukur parameter curah hujan, kelembapan, temperature, intensitas UV,

//Deskripsi penggunaan Pin I/O arduino dengan kompenen diluarnya
//*****************************************************
//  Arduino                   DHT22
//    5V                      VCC
//    D5                   Data(PullUp)
//                             NC
//    GND                     GND
//****************************************************
//  Arduino           Sensor Curah Hujan
//    5V                 Kabel Merah
//    GND                Kabel Hitam
//    D2(Interups)       Kabel Putih
//****************************************************
//  Arduino          Sensor UV (GYML_8511)
//    A0                     OUT
//   3,3V                    3V3
//   GND                     GND
//****************************************************
//  Arduino             Kecepatan Angin
//    A1                 Kabel Kuning
//  12-24V               Kabel Merah
//    GND                Kabel Hitam
//****************************************************
//  Arduino              Arah Angin
//    A2                 Kabel Kuning
//   12-24V              Kabel Merah
//    GND                Kabel Hitam
//****************************************************
//  Arduino         Micro SD Card Adapter
//    D10                   CS
//    D11                  MOSI
//    D12                  MISO
//    D13                  SCK
//    5V                   VCC
//    GND                  GND
//****************************************************
//  Arduino              RTC DS3231
//  SDA Pin                SDA
//  SCL Pin                SCL
//    5V                   VCC
//    GND                  GND
//****************************************************
//  Arduino              ESP 8266
//    D3                    Tx
//    GND                   GND
//   3.3V                  CH_PD
//    EN                   GPIO 2
//    EN                   RESET
//    EN                   GPIO 1
//   3.3V                   VCC
//    D4                    Rx

//****************************************************

//Penggunakan Library
#include <DHT.h>;                 //librari yang digunakan
#include <SD.h>                   //Librari untuk SD Crd
#include <SPI.h>                  //Librari untuk Komunikasi SD Card
#include <DS3231.h>               //Librari untuk RTC
#include <SoftwareSerial.h>       //Librari Software Serial untuk kmunikasi Arduino ke ESP8266

//Definisi Penggunaan IoT
#define WiFiSSID "sonyAl"                   //Nama SSID WiFi router Anda yg terkoneksi internet
#define WiFiPassword "sony1234"             //password router
#define DestinationIP "184.106.153.149"     //website thingspeak.com  (default)
#define TS_Key "DGOJJ2H0LQJGKU8H"           //key yang dihasilkan dari website thingspeak.com
boolean connected = false;                  //Status konesi ke server
#define _rxpin      3                       //Pin Komunikasi Rx ESP
#define _txpin      4                       //Pin Komunikasi Tx ESP
SoftwareSerial debug( _rxpin, _txpin );     // RX, TX

//Definisi dan variabel untuk mengakses Curah Hujan
unsigned long counts;             //variable for GM Tube events
unsigned long previousMillis;     //variable for time measurement
unsigned long currentMillis;
const int bacaCurahHujan = 2;

#define LOG_PERIOD 10000          // Delay pengiriman data

//Definisi dan variabel untuk mengakses DHT
float hum;                        //Variabel penyimpan nilai kelembapan (humidity)
float temp;                       //Variabel penyimpan nilai temperatur

#define DHTPIN 5                  // Pin 5 untuk komunikasi one wire
#define DHTTYPE DHT22             // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);         // Initialize DHT sensor for normal 16mhz Arduino

//Definisi dan variabel untuk mengakses UV sensor
const int ReadUVintensityPin = A0;    //Pin bacaan intensitas cahaya
int uvLevel;
float outputVoltage;
float uvIntensity;

//Definisi dan variabel untuk mengakses Sensor Kecepatan Angin
const int kecepatanAnginPin = A1;
float kecepatanAngin;

//Definisi dan variabel untuk mengakses Sensor Arah Angin
const int arahAnginPin = A2;
float arahAngin;

//Definisi dan variabel untuk mengakses SD Card
File myFile;
int pinCS = 10;           // Pin D10 pada Arduino

//Definisi dan variabel untuk mengakses RTC Module
DS3231  rtc(SDA, SCL);

void setup() {
  //inisialisasi komunikasi Arduino ke ESP8266
  debug.begin(9600);                        //Baudrate Arduino ke ESP8266
  debug.setTimeout(5000);                   //Waktu habis untuk koneksi

  //inisialisasi komunikasi onewire DHT
  dht.begin();

  //inisialisasi curah hujan
  counts = 0;                                                                   //Nilai awal frekuensi curah hujan
  pinMode(bacaCurahHujan, INPUT_PULLUP);                                        // bacaCurahHuja  digunakan mode inpiut pullUp
  attachInterrupt(digitalPinToInterrupt(bacaCurahHujan), impulse, FALLING);     //define external interrupts

  //inisialisai Sensor UV (GYML_8511)
  pinMode(ReadUVintensityPin, INPUT);                                           //A0 sebagai port masukan bacaan UV

  //inisialisai RTC
  rtc.begin();
  // Melakukan set waktu RTC
  rtc.setDOW(WEDNESDAY);          // Untuk mengatur hari
  rtc.setTime(22, 29, 30);        // Untuk mengatur jam : menit : detik     ==> 12:00:00 (24hr format)
  rtc.setDate(4, 19, 2018);       // Untuk mengatur bulan : tanggal : tahun ==> January 1st, 2014

  //inisialisai SD Card
  pinMode(pinCS, OUTPUT);

  // SD Card Initialization
  if (SD.begin())
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }
  rtc.begin();

  // ESP8266 Initialization
  //  periksa apakah modul ESP8266 aktif/Siap
  debug.println("AT+RST");                  //Arduino Melakukan Reset Module ESP8266
  delay(1000);
  if (debug.find("ready")) {                //Jika ESP telah berhasil di reser maka akan mengeluarkan komen "ready"
    Serial.println("Modul siap");
  }
  else {
    Serial.println("Tidak ada respon dari modul");
    while (1);                              //Melakukan pengulangan
  }
  delay(1000);

  //setelah modul siap, kita coba koneksi sebanyak 5 kali
  for (int i = 0; i < 5; i++) {
    connect_to_WiFi();
    if (connected) {        //Jika ESP telah terkoneksi
      break;
    }
  }

  if (!connected) {         //Jika ESP Belum terkoneksi
    while (1);
  }
  delay(5000);

  debug.println("AT+CIPMUX=0");       //Mengaktifkan singgel connection   Ket: jika "AT+CIPMUX=1" maka Mengaktifkan multiple connection
  delay(1000);

  Serial.begin(9600);             //Kecepatan komunikasi UART
}

void loop() {             // Main cycle
  currentMillis = millis();
  if (currentMillis - previousMillis > LOG_PERIOD) {          //Membuat jeda selama 10 detik
    previousMillis = currentMillis;

    //Proses menampilkan ferkuensi curah hujan
    Serial.println(counts);                                   //Menampilkan nilai frekuensi curah hujan selama  10 detik
    //counts = 0;                                               //Mereset nilai frekuensi curah hujan

    //Proses menampilkan nilai humidity dan temperature
    hum = dht.readHumidity();                                 //penyimpanan nilai parameter kelembapan (humidity)
    temp = dht.readTemperature();                             //penyimpanan nilai parameter temperatur
    //menampilkan nilai ke sereial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);                    //meampilkan nilai kelembapan (humidity) ke serial monitor
    Serial.print(" %, Temp: ");
    Serial.print(temp);                   //menampilkan nilai temperatur ke serial monitor
    Serial.println(" Celsius");

    //Proses menampilkan nilai Intensitas UV
    uvLevel = averageAnalogRead(ReadUVintensityPin);              //membaca nilai keluaran tegangan sensor dengan fungsi rata-rata
    outputVoltage = 5.0 * uvLevel / 1024;                         //konversi dari nilai ADC ke tegangan dalam satuan volt
    uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);  //konversi untuk menentukan nilai intensitas dalam satuan mW/cm^2
    //menampikan parameter ke serial monitor
    //Serial.print("UVAnalogOutput: ");
    //Serial.print(uvLevel);

    //Serial.print(" OutputVoltage: ");
    //Serial.print(outputVoltage);

    Serial.print(" UV Intensity: ");
    Serial.print(uvIntensity);
    Serial.println(" mW/cm^2");

    //Proses menampilkan nilai kecepatan Angin
    kecepatanAngin = analogRead(kecepatanAnginPin);
    Serial.print("Kecepatan Angin :");
    Serial.print(kecepatanAngin);
    Serial.println("m/s");

    //Proses menampilkan nilai Arah Angin
    arahAngin = analogRead(arahAnginPin);
    Serial.print("Arah Angin");
    Serial.print(arahAngin);
    Serial.println("drajat");


    //Proses Menampilkan data RTC
    // Mengirin data Hari
    Serial.print(rtc.getDOWStr());
    Serial.print(" ");

    // Mengirim data Tanggal
    Serial.print(rtc.getDateStr());
    Serial.print(" -- ");

    // Mengirim data jam
    Serial.println(rtc.getTimeStr());

    //Proses Menampilkan SC Card
    Serial.print(rtc.getTimeStr());
    Serial.print(",");
    Serial.println(int(rtc.getTemp()));
    Serial.println("kirim ke test.txt");

    myFile = SD.open("test.txt", FILE_WRITE);
    if (myFile) {
      myFile.print(rtc.getTimeStr());
      myFile.print(",");
      myFile.println(int(rtc.getTemp()));
      myFile.close(); // close the file
    }
    // if the file didn't open, print an error:
    else {
      Serial.println("error opening test.txt");
    }


    //Format Melakukan akses halaman WEB ThingSpeak format : AT+CIPSTART=4,"TCP","namaServer",80
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += DestinationIP ;
    cmd += "\",80";
    debug.println(cmd);                     //Mengirim perintah akses ke Server

    //Serial.println(cmd);                  //Menampilkan bahwa server sedang diakses
    if (debug.find("Error")) {              //Jika Akses gagal maka
      Serial.println("Koneksi error.");     //Kirim pemberitahuan gagal ke Laptop
      return;
    }

    //Variabel hasil pembacaan semua sensor
    //hum, temp, uvIntensity, kecepatanAngin, arahAngin, dan count
    cmd = "GET /update?key=";
    cmd += TS_Key;
    cmd += "&field1=";
    cmd += temp;                        //Parameter Temperatur
    cmd += "&field2=";
    cmd += hum;                         //Parameter Humidity
    cmd += "&field3=";
    cmd += uvIntensity;                 //Parameter UV intensity
    cmd += "&field4=";
    cmd += arahAngin;                  //Parameter Arah Angin
    cmd += "&field5=";
    cmd += kecepatanAngin;             //Parameter Kecepatan Angin
    cmd += "&field6=";
    cmd += count;             //Parameter Curah Hujan
    cmd += "\r\n";        // jangan lupa, setiap perintah selalu diakhiri dengan CR+LF
    // Keterangan CR (Carriage Return) Desimal 13,   LF(Line Feed) : new Line decimal 10

    debug.print("AT+CIPSEND=");             //Mengirimkan HTTP GET
    debug.println(cmd.length());
    if (debug.find(">")) {
      //Serial.print(">");
    }
    else {
      debug.println("AT+CIPCLOSE");       //Close TCP/UDP/SSL connection
      //Serial.println("koneksi timeout");
      delay(1000);
      return;
    }

    debug.print(cmd);                    //Mengirimkan semua parameter sesuai cmd
    delay(2000);

    while (debug.available()) {
      char c = debug.read();
      Serial.write(c);
      if (c == '\r') {
        Serial.print('\n');
      }
    }

    Serial.println("------end");        //Proses pengiriman data telah selesai
    //delay(1000);                       //Jeda selama 5 detik


    //Serial.println(" ///////////////////////");
    counts = 0;                                               //Mereset nilai frekuensi curah hujan

  }
}

//Fungsi saat interups dijalankan
void impulse() { // dipanggil setiap ada sinyal FALLING di pin 2
  counts++;
}

//Menentukan rata-rata dari pembacaan UV
int averageAnalogRead(int pinToRead) {
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0 ; x < numberOfReadings ; x++) {
    runningValue += analogRead(pinToRead);    //menjumlahan data pengukuran pertama hingga ke numberOfReading
  }
  runningValue /= numberOfReadings;           // menentukan nilai rata-rata
  return (runningValue);
}

//Fungsi konversi untuk nilai data bertipe float
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Fungsi menghubungkan ke Router atau SSID
void connect_to_WiFi() {
  debug.println("AT+CWMODE=1");         //Keterangan ==> 1 : Station mode,2 : Access Point Mode, dan 3 : Both Mode
  String cmd = "AT+CWJAP=\"";           //Perintah untuk koneksi ke internet  (AT+CWJAP="namawifi","passwordwifi")
  cmd += WiFiSSID;
  cmd += "\",\"";
  cmd += WiFiPassword;
  cmd += "\"";
  //Serial.println(cmd);                //Menampilkan perintah koneksi keinternet
  debug.println(cmd);                   //Arduino memerintahkan ESP untuk koneksi ke internet melalui router
  delay(2000);
  if (debug.find("OK")) {               //Jika "OK" ditemukan maka ESP sudah terkoneksi
    //Serial.println("Sukses, terkoneksi ke WiFi.");
    connected = true;
  }
  else {
    //Serial.println("Tidak dapat terkoneksi ke WiFi. ");
    connected = false;
  }
}
