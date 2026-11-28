#include "funcoes_evtw.h"
#include "esp_netif.h"

bool nat_ativo = false;

void configurarRepetidor() {
    Serial.println("Conectando a rede real (STA) para prover internet...");
    
    // O ESP32 se conecta à rede real
    WiFi.begin(SSID_ALVO, SENHA_REAL);
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n>> Conectado a rede real! IP: " + WiFi.localIP().toString());
        
        // Ativa o NAT (Roteamento)
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif) {
            esp_err_t err = esp_netif_napt_enable(ap_netif);
            if (err == ESP_OK) {
                Serial.println(">> [NAT] Repetidor ativado! Vitimas terao internet.");
                nat_ativo = true;
            } else {
                Serial.println(">> [NAT] Falha ao ativar NAPT.");
            }
        }
    } else {
        Serial.println("\n>> Falha ao conectar na rede real. Sem internet.");
    }
}

void loopRepetidor() {
    static unsigned long ultimo_anuncio = 0;
    unsigned long agora = millis();

    // Se perder a conexão com a internet, pausa o deauth e tenta reconectar.
    // Pausar e' essencial: durante a reassociacao o STA fica vulneravel e o
    // flood poderia impedir o uplink de voltar.
    if (WiFi.status() != WL_CONNECTED) {
        enlaceAnunciar(false);        // go=0 -> deauth pausa
        WiFi.reconnect();
        delay(5000);
        return;
    }

    // Conectado: anuncia o estado ~1x/s e autoriza o deauth (go=1).
    if (agora - ultimo_anuncio >= 1000) {
        ultimo_anuncio = agora;
        enlaceAnunciar(nat_ativo);    // so libera flood com o NAT no ar
    }
}