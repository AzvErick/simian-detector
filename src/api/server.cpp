#include "server.h"
#include "../core/simian_detector.h"
#include "../../third_party/httplib.h"

#include <iostream>
#include <sstream>
#include <algorithm>

// Parser simples pro JSON do request
// Extrai o array de strings do campo "dna"
// Nao usei lib JSON pq o formato eh simples demais pra justificar a dependencia
static std::vector<std::string> parseDnaFromJson(const std::string& json) {
    std::vector<std::string> dna;

    size_t dnaPos = json.find("\"dna\"");
    if (dnaPos == std::string::npos) return dna;

    size_t arrayStart = json.find('[', dnaPos);
    size_t arrayEnd = json.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) return dna;

    std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

    // Pega cada string entre aspas
    size_t pos = 0;
    while ((pos = arrayContent.find('"', pos)) != std::string::npos) {
        size_t endQuote = arrayContent.find('"', pos + 1);
        if (endQuote == std::string::npos) break;

        std::string sequence = arrayContent.substr(pos + 1, endQuote - pos - 1);
        std::transform(sequence.begin(), sequence.end(), sequence.begin(), ::toupper);
        dna.push_back(sequence);

        pos = endQuote + 1;
    }

    return dna;
}

// Monta o JSON de resposta com os detalhes das sequencias
static std::string resultToJson(const SimianResult& result) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"is_simian\": " << (result.isSimian ? "true" : "false") << ",\n";
    ss << "  \"sequences_found\": " << result.matches.size() << ",\n";
    ss << "  \"matches\": [";

    for (size_t i = 0; i < result.matches.size(); i++) {
        const auto& m = result.matches[i];
        if (i > 0) ss << ",";
        ss << "\n    {"
           << "\"start\": [" << m.startRow << ", " << m.startCol << "], "
           << "\"end\": [" << m.endRow << ", " << m.endCol << "], "
           << "\"letter\": \"" << m.letter << "\", "
           << "\"direction\": \"" << m.direction << "\""
           << "}";
    }

    ss << "\n  ]\n}";
    return ss.str();
}

static httplib::Server svr;

Server::Server(const std::string& host, int port, std::shared_ptr<Database> db)
    : host_(host), port_(port), db_(db) {
}

void Server::setupRoutes() {

    // POST /simian - recebe DNA, retorna 200 se simio ou 403 se humano
    svr.Post("/simian", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        auto dna = parseDnaFromJson(req.body);

        if (dna.empty()) {
            res.status = 400;
            res.set_content(
                "{\"error\": \"Invalid request. Send JSON with 'dna' field containing an array of strings.\"}",
                "application/json"
            );
            return;
        }

        std::string validationError = SimianDetector::validate(dna);
        if (!validationError.empty()) {
            res.status = 400;
            res.set_content("{\"error\": \"" + validationError + "\"}", "application/json");
            return;
        }

        SimianResult result = SimianDetector::analyze(dna);

        // Salva no banco se tiver conexao
        if (db_ && db_->isConnected()) {
            db_->saveDnaRecord(dna, result.isSimian);
        }

        std::string jsonResponse = resultToJson(result);
        res.status = result.isSimian ? 200 : 403;
        res.set_content(jsonResponse, "application/json");

        std::cout << "[API] POST /simian -> "
                  << (result.isSimian ? "SIMIAN" : "HUMAN")
                  << " (sequences: " << result.matches.size() << ")" << std::endl;
    });

    // GET /stats - retorna contagens e ratio no formato pedido no teste
    svr.Get("/stats", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        if (!db_ || !db_->isConnected()) {
            res.status = 503;
            res.set_content("{\"error\": \"Database not available\"}", "application/json");
            return;
        }

        DnaStats stats = db_->getStats();

        std::stringstream ss;
        ss << "{"
           << "\"count_mutant_dna\": " << stats.countSimianDna << ", "
           << "\"count_human_dna\": " << stats.countHumanDna << ", "
           << "\"ratio\": " << stats.ratio
           << "}";

        res.status = 200;
        res.set_content(ss.str(), "application/json");

        std::cout << "[API] GET /stats -> simian=" << stats.countSimianDna
                  << " human=" << stats.countHumanDna
                  << " ratio=" << stats.ratio << std::endl;
    });

    // GET /health - pra saber se a API ta no ar e o banco conectado
    svr.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        bool dbOk = db_ && db_->isConnected();
        std::string status = dbOk ? "healthy" : "degraded";

        res.status = 200;
        res.set_content(
            "{\"status\": \"" + status + "\", \"database\": " +
            (dbOk ? "true" : "false") + "}",
            "application/json"
        );
    });

    // CORS preflight - necessario pro frontend conseguir fazer POST
    svr.Options("/simian", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });
}

void Server::start() {
    setupRoutes();

    std::cout << "========================================" << std::endl;
    std::cout << "  Simian Detector API" << std::endl;
    std::cout << "  Running on http://" << host_ << ":" << port_ << std::endl;
    std::cout << "  Endpoints:" << std::endl;
    std::cout << "    POST /simian  - Check DNA" << std::endl;
    std::cout << "    GET  /stats   - Get statistics" << std::endl;
    std::cout << "    GET  /health  - Health check" << std::endl;
    std::cout << "========================================" << std::endl;

    svr.listen(host_.c_str(), port_);
}

void Server::stop() {
    svr.stop();
}
