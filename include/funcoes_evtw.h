#ifndef FUNCOES_EVTW_H
#define FUNCOES_EVTW_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Variáveis globais (definidas no evil_twin.cpp)
extern const char* SSID_ALVO;
extern const char* SENHA_REAL; 
extern uint8_t bssid_alvo[6];
extern int canal_alvo;

// Funções do evil_twin.cpp
void buscarCanalAlvo();

// Funções do deauth.cpp
void configurarDeauth();
void loopDeauth();

// Funções do repetidor.cpp
void configurarRepetidor();
void loopRepetidor();

#endif