#include "database.h"
#include <iostream>
#include <sstream>
#include <functional>
#include <iomanip>
#include <openssl/sha.h>

Database::Database(const std::string& connectionString) {
    conn_ = PQconnectdb(connectionString.c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "[DB] Connection failed: " << PQerrorMessage(conn_) << std::endl;
    } else {
        std::cout << "[DB] Connected to PostgreSQL successfully" << std::endl;
    }
}

Database::~Database() {
    if (conn_) {
        PQfinish(conn_);
        std::cout << "[DB] Connection closed" << std::endl;
    }
}

bool Database::isConnected() const {
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

// Cria a tabela e indices se nao existirem
// dna_hash eh UNIQUE pra garantir que nao duplique registro
bool Database::initialize() {
    if (!isConnected()) {
        std::cerr << "[DB] Cannot initialize: not connected" << std::endl;
        return false;
    }

    const char* createTableSql = R"(
        CREATE TABLE IF NOT EXISTS dna_records (
            id          SERIAL PRIMARY KEY,
            dna_hash    VARCHAR(64) UNIQUE NOT NULL,
            dna_sequence TEXT NOT NULL,
            is_simian   BOOLEAN NOT NULL,
            created_at  TIMESTAMP DEFAULT NOW()
        );

        CREATE INDEX IF NOT EXISTS idx_dna_hash ON dna_records(dna_hash);
        CREATE INDEX IF NOT EXISTS idx_is_simian ON dna_records(is_simian);
    )";

    PGresult* res = PQexec(conn_, createTableSql);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);

    if (!success) {
        std::cerr << "[DB] Failed to create table: " << PQerrorMessage(conn_) << std::endl;
    } else {
        std::cout << "[DB] Table 'dna_records' ready" << std::endl;
    }

    PQclear(res);
    return success;
}

// Insere DNA no banco. Se ja existir (mesmo hash), ignora
// Usa ON CONFLICT DO NOTHING pra garantir unicidade
// Queries parametrizadas ($1, $2, $3) pra evitar sql injection
bool Database::saveDnaRecord(const std::vector<std::string>& dna, bool isSimian) {
    if (!isConnected()) {
        std::cerr << "[DB] Cannot save: not connected" << std::endl;
        return false;
    }

    std::string hash = hashDna(dna);
    std::string jsonDna = dnaToJson(dna);
    std::string simianStr = isSimian ? "true" : "false";

    const char* sql = R"(
        INSERT INTO dna_records (dna_hash, dna_sequence, is_simian)
        VALUES ($1, $2, $3)
        ON CONFLICT (dna_hash) DO NOTHING
    )";

    const char* paramValues[3] = {
        hash.c_str(),
        jsonDna.c_str(),
        simianStr.c_str()
    };

    PGresult* res = PQexecParams(conn_, sql, 3, nullptr,
                                  paramValues, nullptr, nullptr, 0);

    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "[DB] Failed to save DNA: " << PQerrorMessage(conn_) << std::endl;
    }

    PQclear(res);
    return success;
}

// Conta simios e humanos no banco, calcula o ratio
DnaStats Database::getStats() {
    DnaStats stats = {0, 0, 0.0};

    if (!isConnected()) {
        std::cerr << "[DB] Cannot get stats: not connected" << std::endl;
        return stats;
    }

    const char* sql = R"(
        SELECT
            COALESCE(SUM(CASE WHEN is_simian = true THEN 1 ELSE 0 END), 0) as simian_count,
            COALESCE(SUM(CASE WHEN is_simian = false THEN 1 ELSE 0 END), 0) as human_count
        FROM dna_records
    )";

    PGresult* res = PQexec(conn_, sql);

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        stats.countSimianDna = std::stoi(PQgetvalue(res, 0, 0));
        stats.countHumanDna = std::stoi(PQgetvalue(res, 0, 1));

        if (stats.countHumanDna > 0) {
            stats.ratio = static_cast<double>(stats.countSimianDna) /
                          static_cast<double>(stats.countHumanDna);
        }
    } else {
        std::cerr << "[DB] Failed to get stats: " << PQerrorMessage(conn_) << std::endl;
    }

    PQclear(res);
    return stats;
}

// Gera SHA-256 do DNA concatenado com "|" como separador
// Ex: ["ATG", "CGA"] -> SHA256("ATG|CGA")
std::string Database::hashDna(const std::vector<std::string>& dna) const {
    std::string combined;
    for (size_t i = 0; i < dna.size(); i++) {
        if (i > 0) combined += "|";
        combined += dna[i];
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()),
           combined.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(hash[i]);
    }

    return ss.str();
}

// Converte o array de DNA pra string JSON
std::string Database::dnaToJson(const std::vector<std::string>& dna) const {
    std::string json = "[";
    for (size_t i = 0; i < dna.size(); i++) {
        if (i > 0) json += ",";
        json += "\"" + dna[i] + "\"";
    }
    json += "]";
    return json;
}

// Escapa string pra SQL usando a funcao da libpq
std::string Database::escapeString(const std::string& str) const {
    char* escaped = PQescapeLiteral(conn_, str.c_str(), str.size());
    if (!escaped) return "''";
    std::string result(escaped);
    PQfreemem(escaped);
    return result;
}
