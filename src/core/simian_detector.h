#ifndef SIMIAN_DETECTOR_H
#define SIMIAN_DETECTOR_H

#include <string>
#include <vector>

// Guarda onde uma sequencia de 4 letras iguais foi encontrada
struct SequenceMatch {
    int startRow;
    int startCol;
    int endRow;
    int endCol;
    char letter;
    std::string direction; // "horizontal", "vertical", "diagonal_down", "diagonal_up"
};

// Resultado da analise, se eh simio + lista de sequencias encontradas
struct SimianResult {
    bool isSimian;
    std::vector<SequenceMatch> matches;
};

// Classe que faz a deteccao de DNA simio
// em 4 direcoes: horizontal, vertical e duas diagonais
class SimianDetector {
public:
    // retorna true se for simio
    static bool isSimian(const std::vector<std::string>& dna);

    // Mesma logica mas retorna detalhes de todas as sequencias (pro frontend)
    static SimianResult analyze(const std::vector<std::string>& dna);

    // Valida se o DNA eh uma matriz NxN com apenas A, T, C, G
    static std::string validate(const std::vector<std::string>& dna);

private:
    static constexpr int SEQUENCE_LENGTH = 4;

    // Checa se tem 4 letras iguais a partir de (row,col) na direcao (dRow,dCol)
    static bool checkDirection(const std::vector<std::string>& dna,
                               int row, int col,
                               int dRow, int dCol,
                               int n);
};

#endif
