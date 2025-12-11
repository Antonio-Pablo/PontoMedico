#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Wire.h>     // Para comunicação I2C com o RTC
#include <RTClib.h>   // Para o módulo RTC DS1307

// --- Configurações da Rede Wi-Fi ---
const char* ssid = "TIC";
const char* password = "mcgapi1703845";

// --- URL do Google Apps Script ---
const String googleAppsScriptUrl = "https://script.google.com/macros/s/AKfycbz1E-WgNW15UMtByTWxIMjemzluPCulJ2iKhYjQgZ4gUMEuLI2QY4MexWqzCtH4BrCl/exec";

// --- Configurações RFID ---
#define SS_PIN 5
#define RST_PIN 4
#define BUZZER_PIN 2

// --- NOVO: Pino do LED de Status ---
#define STATUS_LED_PIN 16 // Pino digital para o LED de status

MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- Configurações RTC DS1307 ---
RTC_DS1307 rtc; // Objeto para o RTC DS1307

// --- Lista de UIDs e Nomes ---
struct Medico {
    String uid;
    String nome;
};

Medico medicos[] = {
    {"04003D07396C80", "Dr. Pablo"},
    {"04FA8708196C80", "Dra. Raysa Nery"},
    {"04DE3402196C80", "Dr. Ivo Lucas"},
    {"04734506396C80", "Dr. Digeorgio Martins"},
    // Adicione mais médicos conforme necessário
};

String cartaoAdminUID = "313520A3";

void setup() {
    Serial.begin(115200);
    Serial.println();

    // --- NOVO: Inicialização do LED ---
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW); // Começa com o LED desligado

    // --- Inicialização do Buzzer ---
    pinMode(BUZZER_PIN, OUTPUT);

    // --- Inicialização do RFID ---
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Leitor RFID inicializado.");

    // --- Inicialização do RTC ---
    Wire.begin(); // Inicializa a comunicação I2C
    if (!rtc.begin()) {
        Serial.println("ERRO: Não foi possível encontrar o RTC. Verifique as conexões!");
        Serial.println("O horário sera fixo (fallback) ate o RTC ser detectado.");
        // --- NOVO: LED pisca lentamente para indicar erro de RTC ---
        for(int i=0; i<3; i++){ // Pisca 3 vezes para indicar erro no RTC
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(200);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(200);
        }
    } else {
        Serial.println("RTC DS1307 inicializado (se encontrado).");
        // --- IMPORTANTE: Ajuste a hora do RTC apenas uma vez ---
        // Se a hora do RTC estiver errada, descomente UMA das linhas abaixo,
        // faça o upload, e DEPOIS comente-a novamente para não resetar a hora toda vez.
        // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Define a hora pela compilação
        // rtc.adjust(DateTime(2025, 7, 25, 12, 30, 0)); // Define uma data/hora específica (Ano, Mês, Dia, Hora, Minuto, Segundo)
    }

    // --- Conexão Wi-Fi ---
    Serial.print("Conectando-se a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // --- NOVO/ALTERADO: Indica que está tentando conectar ao WiFi piscando o LED ---
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(150); // Pisca mais rápido durante a tentativa de conexão
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN)); // Inverte o estado do LED
        Serial.print(".");

        // Adiciona um timeout para a conexão inicial
        if (millis() - startTime > 30000) { // Tenta por 30 segundos
            Serial.println("\nFalha na conexão WiFi inicial. Verifique suas credenciais e rede.");
            // --- NOVO: Indica erro: LED pisca lentamente e depois apaga ---
            for(int i=0; i<5; i++){ // Pisca 5 vezes lentamente para erro de WiFi
                digitalWrite(STATUS_LED_PIN, HIGH);
                delay(400);
                digitalWrite(STATUS_LED_PIN, LOW);
                delay(400);
            }
            digitalWrite(STATUS_LED_PIN, LOW); // Deixa o LED apagado em caso de falha persistente
            break; // Sai do loop de conexão WiFi
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi conectado!");
        Serial.print("Endereço IP: ");
        Serial.println(WiFi.localIP());
        digitalWrite(STATUS_LED_PIN, HIGH); // LED aceso = Conectado e Pronto
    } else {
        Serial.println("Sistema de presença inicializado, mas WiFi nao conectado. Requer atencao.");
    }

    Serial.println("Sistema de presença pronto!");
}

void loop() {
    // --- NOVO/ALTERADO: Verifica e tenta reconectar o WiFi no loop se desconectado ---
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado, tentando reconectar...");
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN)); // Pisca lentamente se cair
        WiFi.reconnect();
        delay(2000); // Dá um tempo para o ESP32 tentar reconectar
        if(WiFi.status() == WL_CONNECTED){
            digitalWrite(STATUS_LED_PIN, HIGH); // Acende se reconectar
            Serial.println("WiFi reconectado!");
        } else {
             // Se ainda não conectou, continua piscando o LED
             digitalWrite(STATUS_LED_PIN, LOW); // Para garantir que pisca
        }
    } else {
        digitalWrite(STATUS_LED_PIN, HIGH); // Garante que o LED esteja aceso se conectado
    }


    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String tagUID = getUID();
        Serial.print("Cartão detectado: ");
        Serial.println(tagUID);

        if (tagUID == cartaoAdminUID) {
            Serial.println("Cartão de administração detectado. Para gerenciar ou apagar registros, acesse sua planilha Google Sheets.");
            tone(BUZZER_PIN, 2000, 1000); // Som de admin
            delay(1500);
            mfrc522.PICC_HaltA();
            return;
        }

        String nomeMedico = verificarMedico(tagUID);
        if (nomeMedico != "") {
            Serial.println("Médico identificado: " + nomeMedico);

            String tipoRegistro = "Ponto Registrado"; // O Google Apps Script vai interpretar
            
            // --- NOVO: Feedback visual/sonoro antes de enviar (opcional, pode ser movido para sendDataToGoogleSheets) ---
            tone(BUZZER_PIN, 1000, 200); // Curto bip para indicar leitura bem-sucedida do cartão
            delay(250); // Pequena pausa
            
            // --- Enviar Dados para o Google Sheets com Data/Hora do RTC ---
            bool success = sendDataToGoogleSheets(nomeMedico, tagUID, tipoRegistro); // Agora captura o sucesso

            if (success) {
                tone(BUZZER_PIN, 1500, 500); // Som de sucesso mais longo
                Serial.println("Dados enviados com sucesso!");
            } else {
                tone(BUZZER_PIN, 300, 1000); // Som de erro (grave, prolongado)
                Serial.println("Falha no envio dos dados para o Google Sheets.");
            }
            
            delay(1500); // Tempo para o usuário retirar o cartão
        } else {
            Serial.println("Cartão não reconhecido.");
            tone(BUZZER_PIN, 500, 500); // Som de erro para cartão não reconhecido
            delay(1500);
        }

        mfrc522.PICC_HaltA(); // Trava o cartão atual para evitar múltiplas leituras
    }
}

// --- Funções Auxiliares (com pequenas otimizações) ---

String getUID() {
    String tagUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        tagUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    tagUID.toUpperCase();
    return tagUID;
}

String verificarMedico(String uid) {
    for (int i = 0; i < sizeof(medicos) / sizeof(medicos[0]); i++) {
        if (medicos[i].uid == uid) {
            return medicos[i].nome;
        }
    }
    return "";
}

String getDateTimeRTC() {
    // --- NOVO: Verifica se o RTC está rodando antes de tentar ler ---
    if (rtc.isrunning()) {
        DateTime now = rtc.now();
        char buf[20];
        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
        return String(buf);
    } else {
        Serial.println("Aviso: RTC nao esta funcionando, usando horario de fallback.");
        // Poderia adicionar uma lógica para tentar ressincronizar o RTC via NTP aqui,
        // mas para manter a simplicidade, usamos um fallback fixo.
        return "2000-01-01 00:00:00"; // Retorna um horário de fallback se o RTC não estiver rodando
    }
}

// --- ALTERADO: Função sendDataToGoogleSheets agora retorna um booleano para indicar sucesso ---
bool sendDataToGoogleSheets(String nomeMedico, String uid, String tipoRegistro) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(googleAppsScriptUrl);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> doc;
        doc["datahora"] = getDateTimeRTC();
        doc["nome"] = nomeMedico;
        doc["uid"] = uid;
        doc["tipo"] = tipoRegistro;

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.print("Enviando JSON: ");
        Serial.println(jsonPayload);

        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.println("Response: " + response);
            return true; // Sucesso no envio
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            Serial.println("Erro ao enviar dados para o Google Sheets.");
            return false; // Falha no envio
        }
        http.end();
    } else {
        Serial.println("WiFi não conectado ao tentar enviar dados.");
        return false; // Falha (sem WiFi)
    }
}