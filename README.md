# Simian Detector — Teste Técnico CISS

Detector de DNA símio em C++. Analisa uma matriz NxN de bases nitrogenadas (A, T, C, G) e identifica se o DNA pertence a um símio ou humano, com base na presença de 4 ou mais letras iguais consecutivas nas direções horizontal, vertical ou diagonal.

## Estrutura do Projeto

```
simian-detector/
├── CMakeLists.txt                # Build principal (CMake)
├── README.md                     # Este arquivo
├── src/
│   ├── core/
│   │   ├── simian_detector.h     # Header da lógica de detecção
│   │   └── simian_detector.cpp   # Implementação do algoritmo
│   ├── api/
│   │   ├── server.h              # Header da API REST
│   │   └── server.cpp            # Implementação dos endpoints
│   ├── db/
│   │   ├── database.h            # Header da camada de banco
│   │   └── database.cpp          # Implementação PostgreSQL
│   └── main.cpp                  # Entry point da API
├── qt-frontend/
│   ├── mainwindow.h              # Header da janela Qt
│   ├── mainwindow.cpp            # Implementação do frontend
│   └── main.cpp                  # Entry point do frontend
├── tests/
│   └── test_simian.cpp           # Testes unitários (Google Test)
└── third_party/
    └── httplib.h                 # cpp-httplib (HTTP server header-only)
```

## Pré-requisitos

### Ubuntu/Debian

```bash
# Compilador e ferramentas de build
sudo apt update
sudo apt install -y build-essential cmake pkg-config

# PostgreSQL (client library)
sudo apt install -y libpq-dev

# OpenSSL (para hash SHA-256)
sudo apt install -y libssl-dev

# Google Test (para testes unitários)
sudo apt install -y libgtest-dev

# Qt5 Quick/QML (para o frontend — opcional)
sudo apt install -y qtdeclarative5-dev qml-module-qtquick2 qml-module-qtquick-controls2 qml-module-qtquick-layouts qml-module-qtquick-window2

# cpp-httplib (baixar header)
wget -O third_party/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

### Fedora/RHEL

```bash
sudo dnf install -y gcc-c++ cmake pkgconfig libpq-devel openssl-devel gtest-devel qt5-qtbase-devel
```

## Compilação

### API + Testes (sem Qt)

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

### Tudo (API + Testes + Frontend Qt)

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_QT_FRONTEND=ON
make -j$(nproc)
```

## Configuração do PostgreSQL

### 1. Instalar e iniciar o PostgreSQL

```bash
sudo apt install -y postgresql postgresql-contrib
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

### 2. Criar o banco de dados

```bash
sudo -u postgres psql -c "CREATE DATABASE simian_detector;"
sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'postgres';"
```

### 3. Variáveis de ambiente (opcional)

A API lê a configuração do banco via variáveis de ambiente. Os defaults são:

```bash
export DB_HOST=localhost
export DB_PORT=5432
export DB_NAME=simian_detector
export DB_USER=postgres
export DB_PASSWORD=postgres
export API_HOST=0.0.0.0
export API_PORT=8080
```

## Execução

### Rodar a API

```bash
cd build
./simian_api
```

Saída esperada:

```
[DB] Connected to PostgreSQL successfully
[DB] Table 'dna_records' ready
========================================
  Simian Detector API
  Running on http://0.0.0.0:8080
  Endpoints:
    POST /simian  - Check DNA
    GET  /stats   - Get statistics
    GET  /health  - Health check
========================================
```

### Rodar os Testes

```bash
cd build
./simian_tests
```

Ou via CTest:

```bash
cd build
ctest --verbose
```

### Rodar o Frontend Qt

```bash
cd build
./simian_frontend
```

> **Nota:** O frontend precisa que a API esteja rodando para funcionar.

## Uso da API

### POST /simian — Verificar DNA

```bash
# Exemplo: DNA símio
curl -X POST http://localhost:8080/simian \
  -H "Content-Type: application/json" \
  -d '{"dna": ["CTGAGA", "CTGAGC", "TATTGT", "AGAGGG", "CCCCTA", "TCACTG"]}'

# Resposta (HTTP 200):
# {
#   "is_simian": true,
#   "sequences_found": 1,
#   "matches": [
#     {"start": [4, 0], "end": [4, 3], "letter": "C", "direction": "horizontal"}
#   ]
# }
```

```bash
# Exemplo: DNA humano
curl -X POST http://localhost:8080/simian \
  -H "Content-Type: application/json" \
  -d '{"dna": ["ATGCGA", "CAGTGC", "TTATTT", "AGACGG", "GCGTCA", "TCACTG"]}'

# Resposta (HTTP 403):
# {
#   "is_simian": false,
#   "sequences_found": 0,
#   "matches": []
# }
```

### GET /stats — Estatísticas

```bash
curl http://localhost:8080/stats

# Resposta:
# {"count_mutant_dna": 1, "count_human_dna": 1, "ratio": 1.0}
```

### GET /health — Health Check

```bash
curl http://localhost:8080/health

# Resposta:
# {"status": "healthy", "database": true}
```

## Algoritmo

A detecção percorre cada célula da matriz NxN e verifica **4 direções** a partir de cada posição:

1. **Horizontal** → (direita)
2. **Vertical** ↓ (baixo)
3. **Diagonal** ↘ (baixo-direita)
4. **Diagonal** ↙ (baixo-esquerda)

Não verificamos as 8 direções porque as opostas são redundantes (verificar "esquerda" a partir de `[i][j]` é o mesmo que verificar "direita" a partir de `[i][j-3]`).

### Complexidade

- **Tempo:** O(N²) — cada célula é visitada uma vez, com no máximo 4 × 4 = 16 comparações por célula (constante).
- **Espaço:** O(1) auxiliar — não alocamos estruturas proporcionais a N.

## Decisões Técnicas

| Decisão | Motivo |
|---|---|
| **cpp-httplib** | Header-only, zero configuração, tipo "Flask para C++" |
| **libpq (nativa)** | Lib oficial do PostgreSQL, sem ORM desnecessário |
| **SHA-256 para unicidade** | Eficiente e sem colisões para o volume esperado |
| **Queries parametrizadas** | Previne SQL injection |
| **Qt5 Quick/QML no frontend** | Tecnologia usada pela CISS |
| **Google Test** | Framework de testes padrão em C++ |

## Diferenciais Implementados

- **Visualização das sequências:** A API retorna *onde* as sequências foram encontradas (coordenadas), não apenas true/false
- **Frontend Qt:** Interface gráfica que se comunica com a API e destaca visualmente as sequências na grid
- **Validação robusta:** Erros claros para entrada inválida (400 Bad Request)
- **Health check:** Endpoint `/health` para monitoramento
- **CORS habilitado:** Frontend pode rodar em qualquer porta
- **Testes unitários:** 15 casos de teste cobrindo cenários normais e limites
- **Configuração por variáveis de ambiente:** Facilita deploy em diferentes ambientes
