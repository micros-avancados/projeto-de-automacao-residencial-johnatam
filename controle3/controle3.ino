#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

//Wifi Acess Point
String ssid_AP = "AP12";
String password_AP = "4105405478jh";
//MQTT 
char* BROKER_MQTT = "m12.cloudmqtt.com";        //URL do broker MQTT 
int BROKER_PORT = 12394;                        //Porta do Broker MQTT
char* BROKER_ID = "Johnatam Horst2";
char* BROKER_USER =  "grhhohbb";                //Usuário Broker MQTT
char* BROKER_PWD = "C0BAFxCRGHaB";              //Senha Broker MQTT
#define topicoSubscrive "esp8266/controle"
//WiFi Client
const char* ssid_config = "Configuration Esp8266 Sensor";     //ssid REDE WIFI para configuração
const char* password_config = "espadmin";
//Login page
String log_user_config = "admin";                             //user acesso web
String log_password_config = "admin";                         //password acesso web


#define D0    16
#define D1    5
#define IR_LED    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

//Estancias dos Objetos
WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);
IRsend irsend(IR_LED);  
//Variaveis do Systen32
int setpoint = 26;
bool configuration = false;

//Prototipo das funções utilizadas

void loop_config();
void handle_configuration_save();
void handle_setup_page();
void handle_login();
bool is_authentified();
void configuration_ISR();
bool initWiFi();
void initMQTT();
//void reconectWiFi(); 
void answerBroker(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);


void setup() {
  irsend.begin();                                     //Inicializa IRed
  Serial.begin(115200);                               //Inicializa Serial
  pinMode(D0,OUTPUT);
  attachInterrupt(D1, configuration_ISR, RISING);     //interupção para entrar no modo configuração, com a variavel boolleana configuration
 
}

void loop() {
  
  if(configuration){
    WiFi.disconnect(true);
    MQTTServer.disconnect();
    loop_config();         
    }else{
      if(initWiFi()){
      initMQTT();
      }
      VerificaConexoesWiFIEMQTT();
      MQTTServer.loop();
      
    }
}
//************************INTERUPÇÃO EXTERNA*********************************************************
//Interrupção Externa para entrar no modo Configuração
void configuration_ISR(){                                     //nao posso fazer a logica aki, por conta da desativações de varias 
                                                              //funções quando um interupção acontece
  configuration = true;  
  }
//**************FUNÇÃO QUE VERIFICA AUTENTIFICAÇÃO DE LOGIN******************************
//retorna true e false conforme o cookie estiver com o ID correto
bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {           
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}
////***************FUNÇÃO DA PAGINA DE LOGIN***********************************************
////inicialmente o server chamada a paragina de handleconfig, porem o header
////contera a String EPSESSIONID = 0, fazendo chamar esta pagina
void handle_login() {
  String msg;
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")) {                              //adicionar o argumento disconect quando chegar a pagina save
    Serial.println("Disconnection");
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    Serial.println("autenticando");
    if (server.arg("USERNAME") == log_user_config &&  server.arg("PASSWORD") == log_password_config) {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html><body><form action='/login' method='POST'>";
  content += "<br>";
  content += "<center>User:<br><input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "<br>Password:<br><input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<br><br><input type='submit' name='SUBMIT' value='Login'></form>";
  content +=  "<br><h4>"+msg+"</h4>";
  content += "You also can go <a href='/inline'>here</a></body></html>";
  server.send(200, "text/html", content);
}
//
////*****************FUNÇÃO DA PAGINA INICIAL/CONFIGURAÇÃO*********************************
void handle_setup_page(){
 // WiFi.disconnect(true);
Serial.println("Enter handleRoot");
  String header;
  if (!is_authentified()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }else {
  String html = "<html><head><title>Configuration Modulo ESP8266 Sensor</title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";  
  html += "</head><body>";
  html += "<h1><center>Configuration Modulo ESP8266 Sensor</h1>";                     //1 center foi suficiennte
  html += "<p><center>SSID</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";//config_salve
  html += "<input type=text name=ssid_AP placeholder='ssid'/> ";
  html += "<p>Password</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=password_AP placeholder='password'/> ";     
  html += "<p>Sensor SetPoint</p>";                                                   //campo para obter periodo de envio da informação de leitura do sensor
  html += "<form method='POST' action='/'>";  
  html += "<input type=text name=sensor_setpoint placeholder='celsius'/> ";
  html += "<br>";
  html += "<input type=submit name=botao value=Enviar /></p>";  
  html += "</form>"; 
  html += "</body></html>";
  server.send(200, "text/html", html);
  Serial.println(server.arg(ssid_AP));
  }
}
//*****************FUNÇÃO DA PAGINA DE SALVAMENTO DA CONFIGURAÇÃO************************  
  void handle_configuration_save(){
  String html =  "<html><head><title>Saved Settings</title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";
  html += "</head><body>";
  html += "<h1><center>Saved Settings</h1>";
  html += "<h3>...Reset Modulo</h3>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  ssid_AP = server.arg("ssid_AP");                                                //pega valor do html da input com nome = ssid_AP
  password_AP = server.arg("password_AP");                                        //pega valor do html da input com nome = password
  setpoint = server.arg("sensor_setpoint").toInt();                                //pega valor do html da input com nome = sensor_time
  Serial.println(ssid_AP);
  Serial.println(password_AP);
  Serial.println(setpoint);
  configuration = false;
  WiFi.disconnect();
}

//********************FUNÇÃO QUE EXECUTA A PARTE DE CONFIGURAÇÃO***********************
    void loop_config(){
   Serial.println("Configuration is Started!!!");
        delay(2000);                                                        //2 segundos para filtrar ruido do botao
        //nao rola aki //ESP8266WebServer server(80);                       //estancia objeto WebServer, na porta 80, padrao navegadores, nao necessita esta especificada na URL
        WiFi.softAP(ssid_config,password_config,1,!configuration,2);        //cria a rede com o parametro para configuração
        WiFi.softAPConfig(IPAddress(192, 168, 0, 125),    //seta IP Estatico
                          IPAddress(192, 168, 0, 1),      //seta Gateway
                          IPAddress(255, 255, 255, 0));    //mascara
        Serial.println(WiFi.softAPIP());
        server.on("/", handle_setup_page);
        server.on("/login",handle_login);
        server.on("/config_save",handle_configuration_save);
        server.on("/inline", []() {server.send(200, "text/plain", "this works without need of authentification"); });
        const char * headerkeys[] = {"User-Agent", "Cookie"} ;
        size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);  
        server.collectHeaders(headerkeys, headerkeyssize);
        server.begin();
        server.sendHeader("Set-Cookie", "ESPSESSIONID=0");        //precisei forçar esta flag para rodar o login durente teste
        server.handleClient();
        while(configuration){
        server.handleClient();
        }
  }//*******************iNICIALIZA WIFI AO AP**********************************************
//  void initWiFi(){
//    delay(10);
//    Serial.println("------Conexao WI-FI------");
//    Serial.print("Conectando-se na rede: ");
//    Serial.println(ssid_AP);
//    Serial.println("Aguarde");    
//    reconectWiFi();                                               //aqui q realmente executa a conecção
//}
//******************FUNÇÃO QUE CONECTA AO BROKER MQTT*********************************** 
void initMQTT(){
    MQTTServer.setServer(BROKER_MQTT, BROKER_PORT);                     //informa qual broker e porta deve ser conectado, esta função me retorna  o esmo obj PubSubClient
    MQTTServer.setCallback(answerBroker);                               //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
                                                                        //retorna um ponteiro, com um obj PusSubClient    
    MQTTServer.connect(BROKER_ID, BROKER_USER, BROKER_PWD);
    MQTTServer.subscribe(topicoSubscrive);                              //importante esse configuração aki
}
                                              
//*******************FUNÇÃO Q RECONECTA AO BROKER ***************************************
//reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
// em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT(){
  long timer = millis();
    while (!MQTTServer.connected()&&WiFi.status() == WL_CONNECTED){
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        if (MQTTServer.connect(BROKER_ID, BROKER_USER, BROKER_PWD)){            
            MQTTServer.subscribe(topicoSubscrive);
            Serial.println("Conectado com sucesso ao broker MQTT!"); 
        } 
        else{
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
         if((millis()-timer)>4000){
          return;
          }
    }
}
//*****************************RECONECTA WI.FI*****************************************************
//se já está conectado a rede WI-FI, nada é feito. 
//Caso contrário, são efetuadas tentativas de conexão 
bool initWiFi(){
    delay(10);
    //Serial.println("------Conexao WI-FI------");
    //Serial.print("Conectando-se na rede: ");
    //Serial.println(ssid_AP);
    //Serial.println("Aguarde");    
    if (WiFi.status() == WL_CONNECTED){
        //Serial.print("conectado! : ");
        return true;
    }     
    WiFi.begin(ssid_AP.c_str(), password_AP.c_str());                  // Conecta na rede WI-FI    
    Serial.println(ssid_AP);
    Serial.println(password_AP);
    long timer = millis();
    while (WiFi.status() != WL_CONNECTED){
        delay(100);
        Serial.println(".");
        if((millis()-timer)>5000){
          return false;
          }
    }  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
    return true;
}
//*********************FUNÇÃO QUE VERIFICA CONECÇÃO COM O BROKER MQTT******************************
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//Em caso de desconexão (qualquer uma das duas), a conexão é refeita.
void VerificaConexoesWiFIEMQTT(void){
    if (!MQTTServer.connected()){ 
     reconnectMQTT();                                                 //se não há conexão com o Broker, a conexão é refeita
     initWiFi();                                                  //se não há conexão com o WiFI, a conexão é refeita
     }
} 
void answerBroker(char* topic, byte* payload, unsigned int length){
  char resposta[length];
  Serial.println("Resceiver msg!!");
  String msg;
  int valor;
    for(int i = 0; i < length; i++){
       resposta[i] = (char)payload[i];
       msg += resposta[i];
       }
    valor = atoi(resposta);
    if(valor>setpoint){
      digitalWrite(D0,HIGH);                                            //valor > setpoint desligar ar, logica inversa
      irsend.sendNEC(0x00FFE01FUL, 32);
      }else{
        digitalWrite(D0,LOW);
        irsend.sendNEC(0x11FFE01FUL, 32);
        }
    
    Serial.print(msg);  
    Serial.println("      "); 
    Serial.print("Operation Mode is Running, Sensor Setpoint: ");
    Serial.println(setpoint);    
  }
