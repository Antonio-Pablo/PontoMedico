🏥 Sistema de Ponto RFID com ESP32, RTC e Google Sheets 📋 Sobre o Projeto Este projeto implementa um sistema de ponto eletrônico para uso em ambientes como clínicas ou consultórios, utilizando um ESP32 como microcontrolador principal. Ele permite que médicos registrem sua entrada e saída usando cartões RFID e envia esses registros em tempo real para uma planilha no Google Sheets, com a data e hora precisas fornecidas por um módulo RTC DS1307.

A principal característica deste sistema é a simplicidade e a confiabilidade na coleta de dados. A complexidade da análise (como determinar se um registro é de "Entrada" ou "Saída" e calcular as horas trabalhadas) é delegada à planilha do Google Sheets, tornando o dispositivo mais robusto e fácil de manter.

✨ Funcionalidades Leitura RFID: Registra a presença de médicos usando cartões/tags RFID.

Timestamp Preciso: Utiliza um módulo RTC DS1307 para garantir que cada registro tenha uma data e hora exatas, mesmo sem conexão à internet.

Conectividade Wi-Fi: Conecta-se a uma rede Wi-Fi local para enviar os dados.

Integração com Google Sheets: Envia dados (Nome do Médico, UID do Cartão, Data/Hora e Tipo de Registro "Ponto Registrado") para uma planilha Google Sheets via Google Apps Script.

Feedback Visual (LED): Um LED indica o status da conexão Wi-Fi e a prontidão do sistema.

Pisca Rápido: Tentando conectar ao Wi-Fi.

Aceso Contínuo: Wi-Fi conectado e pronto.

Pisca Lento: Erro de conexão Wi-Fi ou RTC.

Feedback Sonoro (Buzzer): Sinaliza a leitura bem-sucedida do cartão, cartão não reconhecido ou falha no envio dos dados.

Gerenciamento Simplificado: A lógica de "Entrada" e "Saída" e o cálculo de horas são feitos posteriormente na própria planilha, proporcionando flexibilidade na análise.

🛠️ Componentes Necessários ESP32 DevKit V1 (ou outro modelo de ESP32 compatível)

Módulo RFID-RC522

Módulo RTC DS1307 (com bateria CR2032 ou similar)

Buzzer Passivo

LED de 5mm (qualquer cor)

Resistor de 220 Ohm (para o LED)

Protoboard e Jumpers

Cartões/Tags RFID (compatíveis com MFRC522, como MIFARE Classic 1K)

Computador com Arduino IDE ou PlatformIO instalado

⚙️ Configuração do Projeto

Google Sheets e Apps Script Crie uma nova planilha no Google Sheets.
Defina as Colunas: Na primeira linha da sua planilha, insira os seguintes cabeçalhos (nesta ordem):

Data/Hora

Nome do Médico

UID do Cartão

Tipo de Registro

Abra o Google Apps Script: Vá em Extensões > Apps Script.

Cole o Código do Apps Script: Substitua o conteúdo padrão pelo script abaixo:

JavaScript

function doPost(e) { var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet(); var data = JSON.parse(e.postData.contents); // Assume que o ESP32 enviará dados JSON

var dataHora = data.datahora; var nomeMedico = data.nome; var uidMedico = data.uid; var tipoRegistro = data.tipo;

sheet.appendRow([dataHora, nomeMedico, uidMedico, tipoRegistro]);

return ContentService.createTextOutput("Registro bem-sucedido!").setMimeType(ContentService.MimeType.TEXT); }

function setupSheet() { // Função opcional para configurar cabeçalhos var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet(); sheet.clear(); sheet.appendRow(["Data/Hora", "Nome do Médico", "UID do Cartão", "Tipo de Registro"]); } Salve o Projeto (ícone de disquete).

Implante o Aplicativo Web:

Clique em Implantar (canto superior direito) > Nova implantação.

Em "Tipo", selecione Aplicativo da Web.

Em "Quem tem acesso", selecione Qualquer pessoa.

Clique em Implantar.

Copie o "URL do aplicativo da Web". Você precisará dele para o código do ESP32.

Configuração do Código do ESP32 (Arduino IDE) Instale as Bibliotecas:
WiFi.h (já vem com a ESP32 Core)

HTTPClient.h (já vem com a ESP32 Core)

SPI.h (já vem com a ESP32 Core)

MFRC522 (Procure por "MFRC522 by UIPEthernet" ou "MFRC522 by Seeed Studio" ou "MFRC522 by Arduin_Libs" no Gerenciador de Bibliotecas)

ArduinoJson (Procure por "ArduinoJson by Benoit Blanchon" no Gerenciador de Bibliotecas)

Wire.h (já vem com a ESP32 Core)

RTClib (Procure por "RTClib by Adafruit" no Gerenciador de Bibliotecas)

Abra o Código do Projeto: Copie e cole o código fornecido anteriormente (a versão "otimizada" com LED e buzzer).

Edite as Credenciais e URLs:

Substitua "SEU_SSID_AQUI" e "SUA_SENHA_AQUI" pelos dados da sua rede Wi-Fi.

Substitua "SUA_NOVA_URL_DO_GOOGLE_APPS_SCRIPT_AQUI" pelo URL que você copiou do Google Apps Script.

Configure os UIDs dos Médicos:

Altere os UIDs e nomes na estrutura medicos[] com os UIDs dos seus próprios cartões RFID e os nomes dos médicos. Você pode descobrir o UID de um cartão usando um sketch simples de "Read NUID" da biblioteca MFRC522.

Defina o cartaoAdminUID se tiver um cartão específico para administração (atualmente ele apenas imprime uma mensagem).

Ajuste o RTC (Apenas na Primeira Vez):

No setup(), localize as linhas rtc.adjust(...).

Descomente APENAS UMA dessas linhas para definir a hora do seu RTC (por exemplo, rtc.adjust(DateTime(F(DATE), F(TIME))); para usar a hora da compilação).

Faça o upload do código para o ESP32.

Após o upload e a confirmação de que o RTC está com a hora certa, COMENTE NOVAMENTE a linha rtc.adjust(...) para que o RTC mantenha a hora sem ser resetado a cada inicialização do ESP32.

Faça o Upload do Código: Selecione a placa "ESP32 Dev Module" e a porta COM correta, e faça o upload do sketch para o seu ESP32.

🚀 Como Usar Ligue o ESP32: Conecte o ESP32 à alimentação.

Observe o LED de Status:

O LED piscará rapidamente enquanto tenta conectar ao Wi-Fi.

Quando conectado, o LED acenderá e permanecerá aceso.

Se houver problemas na conexão ou com o RTC, o LED piscará lentamente ou terá um padrão de erro.

Passe um Cartão RFID: Aproxime um cartão RFID válido do leitor MFRC522.

Feedback Sonoro:

Um bip curto indica que o cartão foi lido.

Um bip mais longo e agudo indica que o registro foi enviado com sucesso para o Google Sheets.

Um som mais grave indica um erro no envio ou que o cartão não foi reconhecido.

Verifique a Planilha: Uma nova linha será adicionada automaticamente à sua planilha do Google Sheets com o registro do ponto, incluindo o nome do médico, UID, "Ponto Registrado" e a data/hora precisa.

📈 Análise de Dados na Planilha (Opcional) Como a lógica de "Entrada" e "Saída" não está no ESP32, você pode usar as poderosas funcionalidades do Google Sheets para analisar os dados:

Tabelas Dinâmicas: Para resumir e agrupar os registros por médico e por dia.

Fórmulas: Você pode criar colunas adicionais na planilha para:

Extrair apenas a data.

Identificar a primeira batida do dia para um médico (Entrada).

Identificar a última batida do dia para um médico (Saída).

Calcular o tempo trabalhado (subtraindo a hora de saída da hora de entrada).

Funções como MINIFS, MAXIFS, QUERY, FILTER são muito úteis aqui.

⚠️ Solução de Problemas LED não acende ou pisca estranho: Verifique as conexões do LED (polaridade e resistor) e a alimentação.

Não conecta ao Wi-Fi: Verifique suas credenciais (ssid e password) no código. Certifique-se de que o ESP32 está dentro do alcance da rede.

"ERRO: Não foi possível encontrar o RTC": Verifique as conexões SDA (GPIO 21) e SCL (GPIO 22) do DS1307. Certifique-se de que a bateria está inserida e funcionando.

Dados não aparecem no Google Sheets:

Verifique o Monitor Serial do ESP32 para mensagens de erro HTTP.

Confirme se a googleAppsScriptUrl no seu código do ESP32 é EXATAMENTE a URL da última implantação do seu Google Apps Script.

Verifique os logs de execução do Google Apps Script (Extensões > Apps Script > Execuções) para ver se o doPost está sendo acionado e se há algum erro.

Certifique-se de que a planilha tem os cabeçalhos das colunas na ordem correta.

Cartão não reconhecido: Verifique o UID do cartão no Monitor Serial e garanta que ele está listado corretamente na estrutura medicos[] no código do ESP32.

🤝 Contribuições Sinta-se à vontade para propor melhorias, corrigir bugs ou adicionar novas funcionalidades. Faça um fork do repositório, implemente suas mudanças e envie um Pull Request!
