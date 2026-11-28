#ifndef FUNCOES_EVTW_H
#define FUNCOES_EVTW_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ---------------------------------------------------------------------------
// Alvo (definido em alvo.cpp) - compartilhado pelos dois firmwares
// ---------------------------------------------------------------------------
extern const char* SSID_ALVO;   // nome da rede legitima a ser atacada
extern const char* SENHA_REAL;  // senha da rede legitima (lab controlado)
extern uint8_t bssid_alvo[6];   // MAC do roteador real (preenchido pelo scan)
extern int canal_alvo;          // canal do roteador real (preenchido pelo scan)

void buscarCanalAlvo();         // alvo.cpp : escaneia e preenche bssid/canal

// ---------------------------------------------------------------------------
// ENLACE UART entre as duas ESP (enlace.cpp)
//
// Ligacao fisica (UART2, cruzada - TX de uma vai no RX da outra):
//   ESP#1 (AP+NAT) GPIO25(TX) -> ESP#2 (deauth) GPIO26(RX)
//   ESP#1 (AP+NAT) GPIO26(RX) <- ESP#2 (deauth) GPIO25(TX)
//   GND <-> GND  (terra comum obrigatorio)
//
// A ESP #1 (AP+NAT) e a dona do estado e anuncia periodicamente o canal, o
// BSSID real (alvo do deauth) e os MACs que o deauth NAO pode atacar: o AP
// falso e o STA do repetidor (uplink da internet). Enquanto o repetidor
// reconecta, ela manda go=0 para pausar o deauth.
// ---------------------------------------------------------------------------
#define PINO_ENLACE_RX  26   // igual nas duas placas (RX recebe do TX da outra)
#define PINO_ENLACE_TX  25   // igual nas duas placas
#define ENLACE_BAUD     115200

// Estado recebido pela ESP de deauth (preenchido por enlaceAtualizarDeauth):
extern volatile uint8_t mac_ap_protegido[6];   // BSSID do AP falso  - nao atacar
extern volatile uint8_t mac_sta_protegido[6];  // STA do repetidor   - nao atacar (uplink)
extern volatile bool    protegidos_validos;    // ja recebemos os MACs a proteger
extern volatile bool    deauth_autorizado;     // master mandou go=1 E protegidos validos

void enlaceIniciar();            // Serial2 nos pinos 25/26 (ambos firmwares)
void enlaceAtualizarDeauth();    // ESP #2: le EVTW, aplica canal/bssid/protegidos/go
void enlaceAnunciar(bool rodar); // ESP #1: envia EVTW ... go=<rodar>

// ---------------------------------------------------------------------------
// ESP #1 - DEAUTH (deauth.cpp)
// ---------------------------------------------------------------------------
void configurarDeauth();
void loopDeauth();

// ---------------------------------------------------------------------------
// ESP #2 - EVIL TWIN + SNIFFER ATIVO (evil_twin.cpp + repetidor.cpp)
// ---------------------------------------------------------------------------
void configurarEvilTwin();
void configurarRepetidor();
void loopRepetidor();

#endif
