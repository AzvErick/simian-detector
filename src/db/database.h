#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <memory>
#include <libpq-fe.h>

// Estatisticas pro endpoint /stats
struct DnaStats {
    int countSimianDna;
    int countHumanDna;
    double ratio;
};

// Camada de acesso ao postgreSQL
// Usa libpq (lib nativa do postgres) direto
class Database {
public:
    explicit Database(const std::string& connectionString);
    ~Database();

    // Nao permite copiar (conexao eh recurso unico)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool initialize();   // cria tabela se nao existir
    bool saveDnaRecord(const std::vector<std::string>& dna, bool isSimian);
    DnaStats getStats();
    bool isConnected() const;

private:
    PGconn* conn_;

    std::string hashDna(const std::vector<std::string>& dna) const;  // SHA-256 pra unicidade
    std::string dnaToJson(const std::vector<std::string>& dna) const;
    std::string escapeString(const std::string& str) const;
};

#endif
