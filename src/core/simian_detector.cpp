#include "simian_detector.h"
#include <algorithm>

// Valida a entrada antes de processar
// Retorna string vazia se OK, ou mensagem de erro
std::string SimianDetector::validate(const std::vector<std::string>& dna) {
    if (dna.empty()) {
        return "DNA array is empty";
    }

    int n = static_cast<int>(dna.size());

    // Checa se eh quadrada (cada linha tem que ter N caracteres)
    for (int i = 0; i < n; i++) {
        if (static_cast<int>(dna[i].size()) != n) {
            return "DNA matrix is not square (NxN). Row " + std::to_string(i) +
                   " has " + std::to_string(dna[i].size()) +
                   " columns, expected " + std::to_string(n);
        }
    }

    // So aceita A, T, C, G
    const std::string validChars = "ATCG";
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            char c = std::toupper(dna[i][j]);
            if (validChars.find(c) == std::string::npos) {
                return "Invalid character '" + std::string(1, dna[i][j]) +
                       "' at position [" + std::to_string(i) + "][" + std::to_string(j) +
                       "]. Only A, T, C, G are allowed";
            }
        }
    }

    return "";
}

// Verifica 4 letras consecutivas a partir de (row,col) na direcao (dRow,dCol)
// Ex: checkDirection(dna, 2, 0, 0, 1, n) -> checa horizontal a partir da linha 2
bool SimianDetector::checkDirection(const std::vector<std::string>& dna,
                                     int row, int col,
                                     int dRow, int dCol,
                                     int n) {
    // Ve se o ponto final cabe na matriz
    int endRow = row + dRow * (SEQUENCE_LENGTH - 1);
    int endCol = col + dCol * (SEQUENCE_LENGTH - 1);

    if (endRow < 0 || endRow >= n || endCol < 0 || endCol >= n) {
        return false;
    }

    // Compara cada letra com a primeira
    char first = std::toupper(dna[row][col]);
    for (int step = 1; step < SEQUENCE_LENGTH; step++) {
        int r = row + dRow * step;
        int c = col + dCol * step;
        if (std::toupper(dna[r][c]) != first) {
            return false;
        }
    }

    return true;
}

// Funcao principal - retorna true se encontrar qualquer sequencia de 4
// So checa 4 direcoes (direita, baixo, diag-baixo-dir, diag-baixo-esq)
// porque as opostas sao redundantes
// Para no primeiro match encontrado (early return)
bool SimianDetector::isSimian(const std::vector<std::string>& dna) {
    std::string error = validate(dna);
    if (!error.empty()) return false;

    int n = static_cast<int>(dna.size());
    if (n < SEQUENCE_LENGTH) return false;

    // {dRow, dCol} para cada direcao
    const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (const auto& dir : directions) {
                if (checkDirection(dna, i, j, dir[0], dir[1], n)) {
                    return true;
                }
            }
        }
    }

    return false;
}

// varre tudo e retorna TODAS as sequencias encontradas
// Usado pelo frontend pra colorir as celulas na grid
SimianResult SimianDetector::analyze(const std::vector<std::string>& dna) {
    SimianResult result;
    result.isSimian = false;

    std::string error = validate(dna);
    if (!error.empty()) return result;

    int n = static_cast<int>(dna.size());
    if (n < SEQUENCE_LENGTH) return result;

    const std::string dirNames[] = {"horizontal", "vertical", "diagonal_down", "diagonal_up"};
    const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int d = 0; d < 4; d++) {
                if (checkDirection(dna, i, j, directions[d][0], directions[d][1], n)) {
                    SequenceMatch match;
                    match.startRow = i;
                    match.startCol = j;
                    match.endRow = i + directions[d][0] * (SEQUENCE_LENGTH - 1);
                    match.endCol = j + directions[d][1] * (SEQUENCE_LENGTH - 1);
                    match.letter = std::toupper(dna[i][j]);
                    match.direction = dirNames[d];

                    result.matches.push_back(match);
                    result.isSimian = true;
                }
            }
        }
    }

    return result;
}
