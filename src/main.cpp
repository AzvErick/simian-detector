#include "api/server.h"
#include "db/database.h"
#include <iostream>
#include <cstdlib>
#include <memory>
#include <csignal>

// Le variavel de ambiente ou retorna o default
static std::string getEnv(const char* name, const std::string& defaultValue) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : defaultValue;
}

static std::unique_ptr<Server> serverPtr;

// Trata Ctrl+C pra encerrar o servidor de forma limpa
void signalHandler(int signum) {
    std::cout << "\n[Main] Shutting down..." << std::endl;
    if (serverPtr) {
        serverPtr->stop();
    }
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Config do banco via variaveis de ambiente (boa pratica pra nao expor senhas)
    std::string dbHost = getEnv("DB_HOST", "localhost");
    std::string dbPort = getEnv("DB_PORT", "5432");
    std::string dbName = getEnv("DB_NAME", "simian_detector");
    std::string dbUser = getEnv("DB_USER", "postgres");
    std::string dbPass = getEnv("DB_PASSWORD", "postgres");

    std::string apiHost = getEnv("API_HOST", "0.0.0.0");
    int apiPort = std::stoi(getEnv("API_PORT", "8080"));

    std::string connStr = "host=" + dbHost +
                          " port=" + dbPort +
                          " dbname=" + dbName +
                          " user=" + dbUser +
                          " password=" + dbPass;

    std::cout << "[Main] Connecting to PostgreSQL at " << dbHost << ":" << dbPort
              << "/" << dbName << std::endl;

    auto db = std::make_shared<Database>(connStr);

    if (db->isConnected()) {
        db->initialize();
    } else {
        std::cerr << "[Main] WARNING: Database not available. "
                  << "API will work without persistence." << std::endl;
    }

    serverPtr = std::make_unique<Server>(apiHost, apiPort, db);
    serverPtr->start();

    return 0;
}
