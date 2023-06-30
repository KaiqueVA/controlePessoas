#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

// I2C ESP SCL = 22 e SDA = 21

struct Date
{
  int dSemana;
  int dia;
  int mes;
  int ano;
  int horas;
  int minutos;
  int segundos;
};

String mensagemTopico1;
String mensagemTopico2;
String mensagemTopico3;

bool cont = false;

const char *mqttServer = "mqtt.eclipseprojects.io";
const int mqttPort = 1883;

char *diaSemana[] = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"};

WiFiUDP ntpUDP;
WiFiClient espClient;

PubSubClient client(espClient);

NTPClient timeClient(ntpUDP, "0.br.pool.ntp.org", -2 * 3600, 60000);

LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

void setupLCD(void);
void conectaWiFi(void);
void setupNTP(void);
void conectaMQTT(void);
void dataLCD(void);
void nomeLCD(String nome);
void dadosLCD(String dados);
void taskVeficacao(void *param);
void callback(char *topic, byte *payload, unsigned int length);
void biometriaSetup(void);
int leituraBiometria(void);
Date getDate(void);

void setup(void)
{
  Serial.begin(115200);
  Serial2.begin(57600);

  setupLCD();

  xTaskCreatePinnedToCore(taskVeficacao, "taskVerificacao", 10000, NULL, 2, NULL, 0);
}

void loop(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    conectaWiFi();
  }

  if (cont == true)
  {
    nomeLCD(mensagemTopico1);
    dadosLCD(mensagemTopico2);
    delay(5000);
    lcd.clear();
    cont = false;
    mensagemTopico1 = "";
    mensagemTopico2 = "";
  }

  dataLCD();
  client.loop();
  delay(1000);
}

void conectaWiFi(void)
{
  Serial.println("Conectando");
  const char *ssid = "xxxxxxxxxx";
  const char *senha = "xxxxxxxxxxx";
  WiFi.begin(ssid, senha);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\n Conectado a rede: ");
  Serial.println(ssid);
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("Conectando a rede:");
  lcd.setCursor(0, 3);
  lcd.print(ssid);
  delay(1000);
}

void taskVeficacao(void *param)
{
  while (1)
  {
    leituraBiometria();
  }
  vTaskDelay(100);
}

void setupNTP(void)
{
  timeClient.begin();

  Serial.println("Aguardando a primeira atualizacao...");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.print("Aguardando dados do horario");
  while (!timeClient.update())
  {
    Serial.print(".");
    timeClient.forceUpdate();
    delay(500);
  }
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("Dados recebidos!");
  delay(1000);
  Serial.print("\nPrimeira atualizacao completa!\n");
  timeClient.setTimeOffset(-10800);
}

void setupLCD(void)
{
  byte digito[]{
      B00000,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B00000,
  };

  byte cdilha[]{
      B01110,
      B10000,
      B10000,
      B10001,
      B01110,
      B00100,
      B00010,
      B00100,
  };

  lcd.init();
  lcd.setBacklight(HIGH);
  lcd.createChar(0, digito);
  lcd.createChar(1, cdilha);
  lcd.setCursor(5, 0);
  lcd.print("Carregando");
  lcd.setCursor(0, 1);
  lcd.print("[");
  lcd.setCursor(19, 1);
  lcd.print("]");
  for (int x = 1; x <= 4; x++)
  {
    lcd.setCursor(x, 1);
    lcd.write(byte(0));
    delay(250);
  }
  biometriaSetup();
  for (int x = 5; x <= 9; x++)
  {
    lcd.setCursor(x, 1);
    lcd.write(byte(0));
    delay(250);
  }
  conectaWiFi();
  for (int x = 10; x <= 14; x++)
  {
    lcd.setCursor(x, 1);
    lcd.write(byte(0));
    delay(250);
  }
  setupNTP();
  for (int x = 14; x <= 15; x++)
  {
    lcd.setCursor(x, 1);
    lcd.write(byte(0));
    delay(250);
  }
  conectaMQTT();
  for (int x = 15; x <= 18; x++)
  {
    lcd.setCursor(x, 1);
    lcd.write(byte(0));
    delay(250);
  }

  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(4, 2);
  lcd.print("Carregado! =D");
  delay(1000);
  lcd.clear();
}

void biometriaSetup(void)
{
  finger.begin(57600);
  if (finger.verifyPassword())
  {
    Serial.println("Sensor biometrico encontrado!");
    lcd.setCursor(0, 2);
    lcd.print("Sensor Biometrico");
    lcd.setCursor(0, 3);
    lcd.print("Encontrado");
    delay(1000);
  }
  else
  {
    Serial.println("Sensor biometrico nao encontrado :(");
    lcd.setCursor(0, 2);
    lcd.print("Sensor Biometrico");
    lcd.setCursor(0, 3);
    lcd.print("Nao encontrado");
    delay(2000);
    while (1)
    {
    }
  }
}

void dataLCD(void)
{

  Date date = getDate();

  lcd.setCursor(7, 0);
  lcd.print(diaSemana[date.dSemana]);
  if (diaSemana[date.dSemana] == "Segunda")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
  }
  if (diaSemana[date.dSemana] == "Terca")
  {
    lcd.setCursor(7, 0);
    lcd.print("Ter");
    lcd.write(byte(1));
    lcd.print("a");
    lcd.print("  ");
  }
  if (diaSemana[date.dSemana] == "Quarta")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
    lcd.print("  ");
  }
  if (diaSemana[date.dSemana] == "Quinta")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
    lcd.print("  ");
  }
  if (diaSemana[date.dSemana] == "Sexta")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
    lcd.print("  ");
  }
  if (diaSemana[date.dSemana] == "Sabado")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
    lcd.print("  ");
  }
  if (diaSemana[date.dSemana] == "Domingo")
  {
    lcd.setCursor(7, 0);
    lcd.print(diaSemana[date.dSemana]);
  }

  lcd.setCursor(0, 1);
  lcd.printf("=====%02d/%02d/%d=====", date.dia, date.mes, date.ano);
  lcd.setCursor(0, 2);
  lcd.printf("======%02d:%02d:%02d======", date.horas, date.minutos, date.segundos);
}

Date getDate(void)
{
  char *strDate = (char *)timeClient.getFormattedDate().c_str();

  Date date;
  sscanf(strDate, "%d-%d-%dT%d:%d:%dZ", &date.ano, &date.mes, &date.dia, &date.horas, &date.minutos, &date.segundos);

  date.dSemana = timeClient.getDay();
  return date;
}

void conectaMQTT(void)
{

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  Serial.println("\nConectando MQTT...");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("Conectando ao MQTT");
  while (!client.connected())
  {
    if (client.connect("Teste_Envio_MQTT"))
    {
      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(0, 2);
      lcd.print("Conectado ao server");
      lcd.setCursor(0, 3);
      lcd.print("MQTT");
      Serial.println("Conectado ao servidor MQTT!");
      client.subscribe("nome_va_teste");
      client.subscribe("dados_va_teste");
      client.subscribe("gravacao_va_teste");
    }
    else
    {
      lcd.setCursor(0, 3);
      lcd.print("Erro na conexao");
      Serial.print("Falha na conexão ao servidor MQTT. Código de erro: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  if (strcmp(topic, "nome_va_teste") == 0)
  {
    Serial.print("Topico nome_va_teste: ");
    Serial.println(messageTemp);
    mensagemTopico1 = messageTemp;
  }
  if (strcmp(topic, "dados_va_teste") == 0)
  {
    Serial.print("Topico dados_va_teste: ");
    Serial.println(messageTemp);
    mensagemTopico2 = messageTemp;
    cont = true;
    lcd.clear();
  }
  if (strcmp(topic, "gravacao_va_teste") == 0)
  {
    Serial.print("gravacao_va_teste: ");
    Serial.println(messageTemp);
    mensagemTopico3 = messageTemp;
  }
}

void nomeLCD(String nome)
{
  lcd.setCursor(0, 0);
  lcd.print(nome);
}

void dadosLCD(String dados)
{
  lcd.setCursor(0, 1);
  lcd.print(dados);
}

int leituraBiometria(void)
{
  unsigned char k = finger.getImage();
  if (k != FINGERPRINT_OK)
    return -1;

  k = finger.image2Tz();
  if (k != FINGERPRINT_OK)
    return -1;

  k = finger.fingerFastSearch();
  if (k != FINGERPRINT_OK)
    return -1;

  Serial.print("Encontrado ID #");
  Serial.println(finger.fingerID);
  String dadosDigitais = String(finger.fingerID);
  if (client.publish("dados_digital", dadosDigitais.c_str()))
  {
    Serial.print("Dado enviado: ");
    Serial.println(dadosDigitais);
  }
  else
  {
    Serial.println("Falha no envio do dado via MQTT");
  }
  delay(5000);

  return finger.fingerID;
}

byte leituraNumero()
{
  int n = 0;

  while (n == 0)
  {
    while (!Serial.available())
      ;
    n = Serial.parseInt();
  }
  return n;
}

byte gravacaoDigital()
{

  byte id;

  int p = -1;
  lcd.clear();
  lcd.print("Aguardando a digital");
  lcd.setCursor(0, 1);
  lcd.print("para gravar no");
  lcd.setCursor(0, 2);
  lcd.print("ID# ");
  lcd.print(id);

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Leitura concluida");
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0, 3);
      lcd.print("Sem dedo ");
      lcd.print("no sensor");
      delay(200);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Erro comunicacao");
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.clear();
      lcd.print("Erro de leitura");
      break;
    default:
      lcd.clear();
      lcd.print("Erro desconhecido");
      break;
    }
  }
  delay(250);
  p = finger.image2Tz(1);
  switch (p)
  {
  case FINGERPRINT_OK:
    lcd.setCursor(0, 1);
    lcd.print("Leitura convertida");
    break;
  case FINGERPRINT_IMAGEMESS:
    lcd.clear();
    lcd.print("Leitura suja");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.print("Erro de comunicação");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    lcd.clear();
    lcd.print("Nao foi possivel");
    lcd.setCursor(0, 1);
    lcd.print("encontrar propriedade");
    lcd.setCursor(0, 2);
    lcd.print("da digital");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    lcd.clear();
    lcd.print("Nao foi possivel");
    lcd.setCursor(0, 1);
    lcd.print("encontrar propriedade");
    lcd.setCursor(0, 2);
    lcd.print("da digital");
    return p;
  default:
    lcd.clear();
    lcd.print("Erro desconhecido");
    return p;
  }

  lcd.setCursor(0, 2);
  lcd.print("Remova o dedo");
  delay(1500);

  p = 0;

  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }

  lcd.clear();
  lcd.print("ID# ");
  lcd.print(id);
  p = -1;
  lcd.setCursor(0, 1);
  lcd.print("Coloque o mesmo dedo");
  lcd.setCursor(0, 2);
  lcd.print("novamente!");

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Leitura concluida");
      break;
    case FINGERPRINT_NOFINGER:
      lcd.setCursor(0, 3);
      lcd.print("Sem dedo ");
      lcd.print("no sensor");
      delay(200);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Erro comunicacao");
      break;
    case FINGERPRINT_IMAGEFAIL:
      lcd.clear();
      lcd.print("Erro de leitura");
      break;
    default:
      lcd.clear();
      lcd.print("Erro desconhecido");
      break;
    }
  }

  p = finger.image2Tz(2);
  switch (p)
  {
  case FINGERPRINT_OK:
    lcd.setCursor(0, 1);
    lcd.print("Leitura convertida");
    break;
  case FINGERPRINT_IMAGEMESS:
    lcd.clear();
    lcd.print("Leitura suja");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.print("Erro de comunicação");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    lcd.clear();
    lcd.print("Nao foi possivel");
    lcd.setCursor(0, 1);
    lcd.print("encontrar propriedade");
    lcd.setCursor(0, 2);
    lcd.print("da digital");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    lcd.clear();
    lcd.print("Nao foi possivel");
    lcd.setCursor(0, 1);
    lcd.print("encontrar propriedade");
    lcd.setCursor(0, 2);
    lcd.print("da digital");
    return p;
  default:
    lcd.clear();
    lcd.print("Erro desconhecido");
    return p;
  }

  delay(1500);
  lcd.clear();
  lcd.print("Criando modela para a");
  lcd.setCursor(0, 1);
  lcd.print("ID #");
  lcd.print(id);

  delay(1500);

  p = finger.createModel();
  switch (p)
  {
  case FINGERPRINT_OK:
    lcd.setCursor(0, 2);
    lcd.print("As digitais batem!");
    break;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.clear();
    lcd.print("Erro de comunicacao");
    return p;
  case FINGERPRINT_ENROLLMISMATCH:
    lcd.clear();
    lcd.print("As digitais nao batem");
    return p;
  default:
    lcd.clear();
    lcd.print("Erro desconhecido");
    return p;
  }
  delay(1500);
  p = finger.storeModel(id);
  switch (p)
  {
  case FINGERPRINT_OK:
    lcd.setCursor(0, 3);
    lcd.print("Armazenado!");
    break;
  case FINGERPRINT_PACKETRECIEVEERR:
    lcd.clear();
    lcd.print("Erro de comunicacao");
    return p;
  case FINGERPRINT_BADLOCATION:
    lcd.clear();
    lcd.print("Nao foi possivel");
    lcd.setCursor(0, 1);
    lcd.print("gravar neste local");
    lcd.setCursor(0, 2);
    lcd.print("da memoria");
    return p;
  case FINGERPRINT_FLASHERR:
    lcd.clear();
    lcd.print("Erro durante a");
    lcd.setCursor(0, 1);
    lcd.print("escrita na memoria");
    lcd.setCursor(0, 2);
    lcd.print("flash");
    return p;
  default:
    lcd.clear();
    lcd.print("Erro desconhecido");
    return p;
  }
  delay(1500);
  lcd.clear();
  return true;
}
