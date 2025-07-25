#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h> // Para facilitar o envio de dados JSON

// --- Configurações da Rede Wi-Fi ---
const char* ssid = "TIC";        // Substitua pelo nome da sua rede Wi-Fi
const char* password = "mcgapi1703845"; // Substitua pela senha da sua rede Wi-Fi

// --- URL do Google Apps Script ---
// Substitua pela URL que você copiou do Passo 1, item 3
const String googleAppsScriptUrl = "https://script.google.com/macros/s/AKfycbz1E-WgNW15UMtByTWxIMjemzluPCulJ2iKhYjQgZ4gUMEuLI2QY4MexWqzCtH4BrCl/exec";

// --- Configurações RFID (mesmos pinos do seu código anterior, adapte se necessário) ---
#define SS_PIN 5   // Pode ser diferente no ESP32, GPIO 5 é comum
#define RST_PIN 4  // Pode ser diferente no ESP32, GPIO 4 é comum
#define BUZZER_PIN 2 // Exemplo de pino para o buzzer

MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- Configurações RTC DS1307 ---
RTC_DS1307 rtc; // Objeto para o RTC DS1307

// --- Lista de UIDs dos cartões RFID e nomes dos médicos associados ---
struct Medico {
    String uid;
    String nome;
};

// **IMPORTANTE:** Mantenha os UIDs em hexadecimal e sem espaços/separadores
// Se os seus UIDs têm mais de 8 caracteres, você precisará adaptar a string de comparação
Medico medicos[] = {
    {"d2c74119", "Dr. Joao Silva"}, // UID do seu cartão de teste
    {"04fa8708196c80", "Dra. Maria Souza"}, // UID do seu outro cartão
    // Adicione mais médicos conforme necessário
};

// UID do cartão especial para apagar registros (não se aplica mais diretamente para SD)
// Manteremos a lógica de "admin" para futuras funcionalidades ou reuso
String cartaoAdminUID = "313520a3"; // Substitua pelo UID do cartão especial

void setup() {
    Serial.begin(115200); // Maior velocidade para o monitor serial do ESP32
    Serial.println(); // Pula uma linha para melhor visualização

    // --- Inicialização do RFID ---
    SPI.begin();       // Inicia a comunicação SPI
    mfrc522.PCD_Init(); // Inicia o módulo MFRC522
    Serial.println("Leitor RFID inicializado.");

    // --- Inicialização do Buzzer ---
    pinMode(BUZZER_PIN, OUTPUT);
    
    // --- Inicialização do RTC ---
    Wire.begin(); // Inicializa a comunicação I2C (para o RTC)
    if (!rtc.begin()) {
        Serial.println("Não foi possível encontrar o RTC. Verifique as conexões!");
        // Opcional: loop infinito para travar se o RTC não for encontrado
        // while (true); 
    }

    // Se a energia foi perdida (bateria do RTC descarregou ou não estava conectada)
    // Você pode definir a hora do RTC aqui (apenas uma vez, então comente depois)
    // Exemplo: rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Define a hora pela compilação do código
    rtc.adjust(DateTime(2025, 07, 25, 11, 10, 0)); // Define uma data e hora específica (Ano, Mês, Dia, Hora, Minuto, Segundo)
    
    Serial.println("RTC DS1307 inicializado.");

    // --- Conexão Wi-Fi ---
    Serial.print("Conectando-se a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());

    Serial.println("Sistema de presença pronto!");
}

void loop() {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String tagUID = getUID(); // Obtém o UID do cartão
        Serial.print("Cartão detectado: ");
        Serial.println(tagUID);

        // --- Lógica de Cartão Admin (adaptada, pois não há SD para apagar) ---
        if (tagUID == cartaoAdminUID) {
            Serial.println("Cartão de administração detectado. Funcionalidade de apagar registros não implementada para a nuvem.");
            Serial.println("Para gerenciar registros, acesse sua planilha Google Sheets.");
            tone(BUZZER_PIN, 2000, 1000); // Som de admin
            delay(1500); // Pequena pausa para evitar leitura dupla imediata
            mfrc522.PICC_HaltA(); // Para o PICC (card)
            return; 
        }

        // --- Verifica se o UID está associado a um médico ---
        String nomeMedico = verificarMedico(tagUID);
        if (nomeMedico != "") {
            Serial.println("Médico identificado: " + nomeMedico);

            // Esta parte da lógica "entrada/saída" será simplificada para envio direto
            // do tipo de ponto. O controle de quem está "dentro" ou "fora"
            // será gerenciado pela análise da planilha no Google Sheets.
            
            // Para manter a lógica de alternar "entrada/saída" no dispositivo
            // precisaríamos de um armazenamento local (ex: LittleFS) para guardar o último estado
            // de cada UID. Por agora, vamos registrar como "Entrada" e "Saída" sequencialmente
            // ou sempre como "Entrada" e deixar a lógica de pareamento para a planilha.
            // Para um sistema de ponto robusto, você precisaria de um banco de dados
            // que faça essa verificação ou um sistema mais sofisticado no ESP32.
            
            // POR SIMPLICIDADE INICIAL: vamos sempre registrar como "Entrada"
            // ou você pode ter um botão para "Entrada" e outro para "Saída"
            // Para o MVP, vamos considerar apenas "Ponto Registrado" ou pedir confirmação manual.

            // Por enquanto, vamos registrar a ação como "Ponto Registrado" e enviar.
            // A lógica de "Entrada" ou "Saída" pode ser deduzida na planilha
            // analisando o registro anterior para aquele UID.
            // Ou, para ser explícito, podemos adicionar uma flag aqui.
            // Por hora, vamos simplificar para "Ponto" e pedir que o usuário
            // decida se quer um "Entrada" e "Saída" mais inteligente aqui.

            // Para simular Entrada/Saída baseada em um estado simples:
            // Isso é um placeholder. Para um controle robusto, usaríamos um armazenamento persistente.
            static bool isEntrada = true; // Simplesmente alterna. NÃO persistente!
            String tipoRegistro = isEntrada ? "Entrada" : "Saida";
            isEntrada = !isEntrada; // Inverte para a próxima vez

            // --- Enviar Dados para o Google Sheets ---
            sendDataToGoogleSheets(nomeMedico, tagUID, tipoRegistro);
            
            tone(BUZZER_PIN, 1000, 500); // Som de sucesso
            delay(1500); // Pequena pausa para evitar leitura dupla imediata
        } else {
            Serial.println("Cartão não reconhecido.");
            tone(BUZZER_PIN, 500, 500); // Som de erro
            delay(1500); // Pequena pausa
        }

        mfrc522.PICC_HaltA(); // Para o PICC (card)
    }
}

// --- Funções Auxiliares ---

// Função para obter o UID do cartão RFID
String getUID() {
    String tagUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        // Formata o byte em hexadecimal, adicionando '0' se for menor que 0x10
        tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        tagUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    tagUID.toUpperCase(); // Converte para maiúsculas para padronizar
    return tagUID;
}

// Função para verificar se o UID está associado a um médico
String verificarMedico(String uid) {
    for (int i = 0; i < sizeof(medicos) / sizeof(medicos[0]); i++) {
        if (medicos[i].uid == uid) {
            return medicos[i].nome;
        }
    }
    return ""; // Retorna vazio se o UID não for encontrado
}

// Função para obter a data e hora do RTC (DS1307)
String getDateTimeRTC() {
    DateTime now = rtc.now(); // Lê a data e hora atual do RTC
    char buf[20]; // Buffer para formatar a string
    // Formata a data e hora como "YYYY-MM-DD HH:MM:SS"
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    return String(buf);
}

// Função para enviar os dados para o Google Sheets
void sendDataToGoogleSheets(String nomeMedico, String uid, String tipoRegistro) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(googleAppsScriptUrl);
        http.addHeader("Content-Type", "application/json"); // Indicamos que estamos enviando JSON

        // Cria o objeto JSON
        StaticJsonDocument<200> doc; // Tamanho do documento JSON (pode precisar ajustar)
        doc["datahora"] = getDateTimeRTC();
        doc["nome"] = nomeMedico;
        doc["uid"] = uid;
        doc["tipo"] = tipoRegistro;

        String jsonPayload;
        serializeJson(doc, jsonPayload); // Converte o objeto JSON para String

        Serial.print("Enviando JSON: ");
        Serial.println(jsonPayload);

        int httpResponseCode = http.POST(jsonPayload); // Envia a requisição POST com o JSON

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi não conectado ao tentar enviar dados.");
    }
}
