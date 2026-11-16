#include "funcoes_evtw.h"

const char* SSID_ALVO = "Senac-Adm"; 
const char* SENHA_REAL = "bemvindoaocesar"; 

int canal_alvo = 1; 
uint8_t bssid_alvo[6] = {0, 0, 0, 0, 0, 0};

void buscarCanalAlvo() {
    Serial.println("Escaneando alvo para obter BSSID e Canal...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks();
    bool encontrou = false;

    for (int i = 0; i < n; ++i) {
        if (String(WiFi.SSID(i)) == String(SSID_ALVO)) {
            canal_alvo = WiFi.channel(i);
            memcpy(bssid_alvo, WiFi.BSSID(i), 6); 
            
            Serial.print(">> Alvo Encontrado! Canal: "); Serial.println(canal_alvo);
            Serial.print(">> BSSID: ");
            for (int j = 0; j < 6; j++) {
                Serial.printf("%02X", bssid_alvo[j]);
                if (j < 5) Serial.print(":");
            }
            Serial.println();
            encontrou = true;
            break; 
        }
    }
    WiFi.scanDelete(); 
    if (!encontrou) {
        Serial.println(">> AVISO: Alvo nao encontrado.");
    }
}