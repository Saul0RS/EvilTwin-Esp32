#include "funcoes_evtw.h"

// ---------------------------------------------------------------------------
// Configuracao do alvo (rede legitima da propria equipe, em laboratorio)
// ---------------------------------------------------------------------------
const char* SSID_ALVO  = "GlubGlub";
const char* SENHA_REAL = "congratulations";

// Preenchidos em tempo de execucao pelo scan:
int canal_alvo = 1;
uint8_t bssid_alvo[6] = {0, 0, 0, 0, 0, 0};

// Escaneia o ambiente, acha o SSID alvo e guarda o BSSID (MAC) e o canal reais.
// Usado tanto pela ESP de deauth quanto pela Evil Twin.
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

            Serial.print(">> Alvo encontrado! Canal: ");
            Serial.println(canal_alvo);
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
        Serial.println(">> AVISO: alvo nao encontrado. Deauth/Evil Twin nao terao efeito.");
    }
}
