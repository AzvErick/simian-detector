# Simian Detector — Teste Técnico CISS

Detector de DNA símio em C++. Analisa uma matriz NxN de bases nitrogenadas (A, T, C, G) e identifica se o DNA pertence a um símio ou humano, com base na presença de 4 ou mais letras iguais consecutivas nas direções horizontal, vertical ou diagonal.

## API em Produção

A API está hospedada no Google Cloud e acessível publicamente:

```
Base URL: http://34.69.179.211:8080
```

Endpoints disponíveis:

```bash
# Health check
curl http://34.69.179.211:8080/health

# Verificar DNA (símio → 200, humano → 403)
curl -X POST http://34.69.179.211:8080/simian \
  -H "Content-Type: application/json" \
  -d '{"dna": ["CTGAGA", "CTGAGC", "TATTGT", "AGAGGG", "CCCCTA", "TCACTG"]}'

# Estatísticas
curl http://34.69.179.211:8080/stats
```

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
│   ├── dnacontroller.h           # Ponte C++/QML (header)
│   ├── dnacontroller.cpp         # Ponte C++/QML (implementação)
│   ├── main.qml                  # Interface declarativa em QML
│   ├── main.cpp                  # Entry point do frontend
│   └── resources.qrc             # Recursos embutidos no executável
├── tests/
│   └── test_simian.cpp           # Testes unitários (Google Test)
└── third_party/
    └── httplib.h                 # cpp-httplib (HTTP server header-only)
```

## Pré-requisitos

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
  libpq-dev libssl-dev libgtest-dev \
  qtdeclarative5-dev qml-module-qtquick2 \
  qml-module-qtquick-controls2 qml-module-qtquick-layouts \
  qml-module-qtquick-window2

# cpp-httplib (baixar header)
wget -O third_party/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

## Compilação

### API + Testes (sem frontend)

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

### Tudo (API + Testes + Frontend QML)

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_QT_FRONTEND=ON
make -j$(nproc)
```

## Configuração do PostgreSQL

```bash
sudo apt install -y postgresql postgresql-contrib
sudo systemctl start postgresql
sudo systemctl enable postgresql
sudo -u postgres psql -c "CREATE DATABASE simian_detector;"
sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'postgres';"
```

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

### Rodar a API localmente

```bash
cd build
./simian_api
```

### Rodar os Testes

```bash
cd build
./simian_tests
```

### Rodar o Frontend QML

```bash
cd build
./simian_frontend
```

O frontend se conecta à API hospedada no Google Cloud automaticamente.

## Uso da API

### POST /simian — Verificar DNA

```bash
# DNA símio (retorna HTTP 200)
curl -X POST http://34.69.179.211:8080/simian \
  -H "Content-Type: application/json" \
  -d '{"dna": ["CTGAGA", "CTGAGC", "TATTGT", "AGAGGG", "CCCCTA", "TCACTG"]}'

# Resposta:
# {
#   "is_simian": true,
#   "sequences_found": 1,
#   "matches": [
#     {"start": [4, 0], "end": [4, 3], "letter": "C", "direction": "horizontal"}
#   ]
# }
```

```bash
# DNA humano (retorna HTTP 403)
curl -X POST http://34.69.179.211:8080/simian \
  -H "Content-Type: application/json" \
  -d '{"dna": ["ATGCGA", "CAGTGC", "TTATTT", "AGACGG", "GCGTCA", "TCACTG"]}'

# Resposta:
# {
#   "is_simian": false,
#   "sequences_found": 0,
#   "matches": []
# }
```

### GET /stats — Estatísticas

```bash
curl http://34.69.179.211:8080/stats

# Resposta:
# {"count_mutant_dna": 2, "count_human_dna": 3, "ratio": 0.666667}
```

### GET /health — Health Check

```bash
curl http://34.69.179.211:8080/health

# Resposta:
# {"status": "healthy", "database": true}
```

## Algoritmo

A detecção percorre cada célula da matriz NxN e verifica 4 direções a partir de cada posição: horizontal (→), vertical (↓), diagonal (↘) e diagonal (↙).

Não verificamos as 8 direções porque as opostas são redundantes (verificar "esquerda" a partir de `[i][j]` é o mesmo que verificar "direita" a partir de `[i][j-3]`).

### Complexidade

- **Tempo:** O(N²) — cada célula é visitada uma vez, com no máximo 16 comparações por célula (constante).
- **Espaço:** O(1) auxiliar.

## Decisões Técnicas

| Decisão | Motivo |
|---|---|
| **C++17** | Features modernas sem perder compatibilidade |
| **cpp-httplib** | Header-only, zero configuração |
| **libpq** | Lib oficial do PostgreSQL, sem ORM desnecessário |
| **SHA-256 para unicidade** | Eficiente e sem colisões para o volume esperado |
| **Queries parametrizadas** | Previne SQL injection |
| **Qt Quick/QML** | Tecnologia usada pela CISS no dia a dia |
| **Google Test** | Framework de testes padrão em C++ |
| **Google Cloud** | Hospedagem da API em produção |

## Diferenciais

- **API em produção** hospedada no Google Cloud
- **Frontend QML** com grid visual que destaca as sequências encontradas com cores diferentes
- **Detecção de sobreposições** quando múltiplas sequências passam pela mesma célula
- **Resposta detalhada** da API com coordenadas das sequências (não apenas true/false)
- **Validação robusta** com mensagens de erro claras (400 Bad Request)
- **Testes unitários** cobrindo cenários normais e casos limite
- **Unicidade no banco** garantida via SHA-256 + ON CONFLICT DO NOTHING
- **Configuração por variáveis de ambiente** para facilitar deploy
