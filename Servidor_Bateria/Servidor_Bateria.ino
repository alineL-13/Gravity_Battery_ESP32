#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <vector>
#include <algorithm>

// ====== CONFIG GERAIS ======
#define REF_VOLTAGE 3.3
#define ADC_RESOLUTION 4095.0
#define R1 30000.0 // ohms
#define R2 7500.0  // ohms

WebServer server(80);

// ====== CONFIG WIFI ======
const char* ssid = "nomeWifi";
const char* password = "senhaWifi";

// ====== PINOS ======
const int voltageSensorPin = 35; // tensão

// ====== VARIÁVEIS GLOBAIS ======
struct Energia {
  float voltagem;
  time_t data;
  Energia* esquerda;
  Energia* direita;
};

Energia* raiz = NULL;

String voltageData = "0";
unsigned long lastUpdate = 0;

// ====== HEADERS DAS FUNÇÕES ======
void handleRoot();
void handleData();
void handleHistory();
void updateSensorData();
float readVoltage();
Energia* criarNovoNo(float voltage);
Energia* inserirNaBST(Energia* raiz, Energia* novoNo);
String gerarJSONDaArvore(Energia* raiz);

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  configTime(-3 * 3600, 0, "pool.ntp.org");

  analogReadResolution(12);
  analogSetPinAttenuation(voltageSensorPin, ADC_11db);

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/history", handleHistory);
  server.begin();
  Serial.println("Servidor Web iniciado!");
}

// ====== LOOP ======
void loop() {
  server.handleClient();

  if (millis() - lastUpdate > 500) {
    updateSensorData();
    lastUpdate = millis();
  }
}

// ====== LEITURA DE TENSÃO ======
float readVoltage() {
  int samples = 20;
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(voltageSensorPin);
    delay(1);
  }

  float adcAverage = sum / (float)samples;
  float voltage = (adcAverage * REF_VOLTAGE) / ADC_RESOLUTION;
  float realVoltage = voltage * ((R1 + R2) / R2);

  if (realVoltage < 0.1) realVoltage = 0.0; // Filtragem para evitar ruído do pino flutuante

  return realVoltage;
}

// ====== ÁRVORE BINÁRIA ======
Energia* criarNovoNo(float voltage) {
  Energia* novoNo = (Energia*)malloc(sizeof(Energia));
  if (novoNo == NULL) return NULL;
  novoNo->voltagem = voltage;
  novoNo->data = time(nullptr);
  novoNo->esquerda = NULL;
  novoNo->direita = NULL;
  return novoNo;
}

Energia* inserirNaBST(Energia* raiz, Energia* novoNo) {
  if (raiz == NULL) return novoNo;
  if (novoNo->voltagem < raiz->voltagem)
    raiz->esquerda = inserirNaBST(raiz->esquerda, novoNo);
  else if (novoNo->voltagem > raiz->voltagem)
    raiz->direita = inserirNaBST(raiz->direita, novoNo);
  return raiz;
}

// ====== GERAR JSON DA ÁRVORE ======
void coletarNos(Energia* raiz, std::vector<Energia*>& lista) {
  if (!raiz) return;
  lista.push_back(raiz);
  coletarNos(raiz->esquerda, lista);
  coletarNos(raiz->direita, lista);
}

String gerarJSONDaArvore(Energia* raiz) {
  std::vector<Energia*> lista;
  coletarNos(raiz, lista);

  std::sort(lista.begin(), lista.end(), [](Energia* a, Energia* b) {
    return difftime(a->data, b->data) > 0;
  });

  String json = "[";
  for (size_t i = 0; i < lista.size(); i++) {
    Energia* e = lista[i];
    struct tm* timeinfo = localtime(&e->data);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);

    json += "{";
    json += "\"hora\":\"" + String(buffer) + "\",";
    json += "\"voltage\":\"" + String(e->voltagem, 2) + "\"";
    json += "}";
    if (i < lista.size() - 1) json += ",";
  }
  json += "]";
  return json;
}

// ====== ROTAS DO SERVIDOR ======
void handleHistory() {
  String json = gerarJSONDaArvore(raiz);
  server.send(200, "application/json", json);
}

// ====== ATUALIZAÇÃO DE DADOS ======
void updateSensorData() {
  float voltage = readVoltage();
  voltageData = String(voltage, 2);

  Energia* novoNo = criarNovoNo(voltage);
  raiz = inserirNaBST(raiz, novoNo);

  Serial.printf("Tensão: %.2f V\n", voltage);
}

// ====== HTML DA INTERFACE ======
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <title>Monitor de Tensão</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- Bootstrap CSS -->
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
  <!-- Chart.js -->
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    .gradient-card {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      border: none;
      border-radius: 15px;
      transition: transform 0.3s ease;
    }
    .gradient-card:hover {
      transform: translateY(-5px);
    }
    .data-value {
      font-size: 2.5rem;
      font-weight: bold;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    .chart-container {
      background: white;
      border-radius: 15px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    body {
      background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
      min-height: 100vh;
      padding: 20px 0;
    }
    .header-section {
      background: white;
      border-radius: 15px;
      padding: 30px;
      margin-bottom: 30px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
  </style>
</head>
<body>
  <div class="container">
    <!-- Cabeçalho -->
    <div class="header-section text-center">
      <h1 class="display-4 text-primary mb-3">Monitor de Tensão</h1>
      <p class="lead text-muted">Monitoramento em tempo real de Tensão</p>
    </div>

    <!-- Card de Dados e Gráfico-->
    <div class="row g-4 mb-5 justify-content-center">
      <div class="col-md-3">
        <div class="card gradient-card text-white text-center py-4">
          <div class="card-body">
            <h5 class="card-title">Tensão</h5>
            <div class="data-value" id="v">0.00</div>
            <p class="card-text mb-0">Volts</p>
          </div>
        </div>
      </div>

      <div class="col-md-9">
        <div class="chart-container">
          <canvas id="chartV"></canvas>
        </div>
      </div>
    </div>

    <div class="row mt-5">
      <div class="col-12">
        <div class="chart-container">
          <h4 class="mb-3 text-center">Histórico de Medições</h4>
          <table class="table table-striped text-center align-middle" id="historyTable">
            <thead class="table-primary">
              <tr>
                <th>Hora</th>
                <th>Tensão (V)</th>
              </tr>
            </thead>
            <tbody></tbody>
          </table>
        </div>
      </div>
    </div>

  <!-- Bootstrap JS -->
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js"></script>

  <script>
    // Inicialização do gráfico
    const ctxV = document.getElementById('chartV').getContext('2d');
    
    const chartV = new Chart(ctxV, {
      type: 'line',
      data: { 
        labels: [], 
        datasets: [{ 
          label: 'Tensão (V)', 
          data: [], 
          borderColor: '#4CAF50', 
          backgroundColor: '#4CAF5020',
          tension: 0.4, 
          fill: true,
          borderWidth: 3,
          pointRadius: 0
        }] 
      },
      options: { 
        responsive: true, 
        maintainAspectRatio: false,
        animation: { duration: 0 },
        scales: { 
          y: { 
            beginAtZero: true,
            grid: { color: 'rgba(0,0,0,0.1)' }
          },
          x: {
            grid: { color: 'rgba(0,0,0,0.1)' }
          }
        },
        plugins: {
          legend: { 
            labels: { font: { size: 14 } }
          }
        }
      }
    });

    let time = 0;
    const maxPoints = 30;

    // Função para atualizar dados
    async function updateData() {
      try {
        const response = await fetch('/data');
        const data = await response.json();
        
        // Atualiza o valor no card
        document.getElementById('v').textContent = data.voltage;

        // Atualiza o gráfico
        updateChart(chartV, parseFloat(data.voltage));
        
      } catch (error) {
        console.error('Erro ao buscar dados:', error);
      }
    }

    // Função para atualizar gráficos
    function updateChart(chart, value) {
      if (chart.data.labels.length >= maxPoints) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
      }
      
      chart.data.labels.push(new Date().toLocaleTimeString());
      chart.data.datasets[0].data.push(value);
      chart.update('none');
    }

    // Atualiza a cada 500ms
    setInterval(updateData, 500);
    
    // Primeira atualização
    updateData();

    async function updateHistory() {
      try {
        const response = await fetch('/history');
        const data = await response.json();

        const tbody = document.querySelector('#historyTable tbody');
        tbody.innerHTML = '';

        data.forEach(row => {
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td>${row.hora}</td>
            <td>${row.voltage}</td>
          `;
          tbody.appendChild(tr);
        });
      } catch (error) {
        console.error('Erro ao buscar histórico:', error);
      }
    }

    // Atualiza a tabela a cada 2 segundos
    setInterval(updateHistory, 2000);
    updateHistory();
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ====== JSON COM DADOS ======
void handleData() {
  String json = "{";
  json += "\"voltage\":\"" + voltageData + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
