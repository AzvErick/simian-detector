#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include "../db/database.h"

// Servidor HTTP da API REST
// Endpoints: POST /simian, GET /stats, GET /health
class Server {
public:
    // db eh opcional - se for nullptr, roda sem persistencia
    Server(const std::string& host, int port, std::shared_ptr<Database> db = nullptr);

    void start(); // bloqueia ate chamar stop()
    void stop();

private:
    std::string host_;
    int port_;
    std::shared_ptr<Database> db_;

    void setupRoutes();
};

#endif
