#include <gtest/gtest.h>
#include "../src/core/simian_detector.h"

//Google test, testes unitarios

// --- Validacao ---

TEST(SimianValidation, EmptyDna) {
    std::vector<std::string> dna = {};
    EXPECT_FALSE(SimianDetector::validate(dna).empty()); // deve dar erro
}

TEST(SimianValidation, NonSquareMatrix) {
    std::vector<std::string> dna = {"ATGCGA", "CAGTGC", "TTATGT"}; // 3x6, nao eh NxN
    EXPECT_FALSE(SimianDetector::validate(dna).empty());
}

TEST(SimianValidation, InvalidCharacters) {
    std::vector<std::string> dna = {"ATXG", "CAGG", "TTAG", "AGAG"}; // X invalido
    EXPECT_FALSE(SimianDetector::validate(dna).empty());
}

TEST(SimianValidation, ValidDna) {
    std::vector<std::string> dna = {"ATGC", "CAGG", "TTAG", "AGAT"};
    EXPECT_TRUE(SimianDetector::validate(dna).empty()); // sem erro
}

// Deteccao: casos SIMIO (true)

// Exemplo do enunciado, tem CCCC horizontal na linha 5
TEST(SimianDetection, ExampleFromSpec) {
    std::vector<std::string> dna = {
        "CTGAGA", "CTGAGC", "TATTGT", "AGAGGG", "CCCCTA", "TCACTG"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// AAAA horizontal na primeira linha
TEST(SimianDetection, HorizontalSequence) {
    std::vector<std::string> dna = {
        "AAAATG", "CAGTGC", "TTATGT", "AGACGG", "GCGTCA", "TCACTG"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// 4 letras T na coluna 2 (vertical)
TEST(SimianDetection, VerticalSequence) {
    std::vector<std::string> dna = {
        "ATGCGA", "CATGCC", "GTTAGT", "GATGGG", "ACTCTA", "TCACTG"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// Diagonal pra baixo-direita
TEST(SimianDetection, DiagonalDownRight) {
    std::vector<std::string> dna = {
        "ATGCGA", "CAGTGC", "TGATGT", "AGTAAG", "GCGTCA", "TCACTG"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// Diagonal pra baixo-esquerda
TEST(SimianDetection, DiagonalDownLeft) {
    std::vector<std::string> dna = {
        "ATGCGG", "CAGTGC", "TTGGGT", "AGGAAG", "GCGTCA", "TCACTG"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// Multiplas sequencias - analyze() deve achar mais de uma
TEST(SimianDetection, MultipleSequences) {
    std::vector<std::string> dna = {
        "AAAATG", "AATATC", "AATTGT", "AATAGG", "CCCCTA", "TCACTG"
    };
    SimianResult result = SimianDetector::analyze(dna);
    EXPECT_TRUE(result.isSimian);
    EXPECT_GT(result.matches.size(), 1u);
}

// Deteccao: casos HUMANO (false)

TEST(SimianDetection, HumanDna) {
    std::vector<std::string> dna = {
        "ATGCGA", "CAGTGC", "TTATTT", "AGACGG", "GCGTCA", "TCACTG"
    };
    EXPECT_FALSE(SimianDetector::isSimian(dna));
}

// 4x4 sem nenhuma sequencia
TEST(SimianDetection, MinimalMatrixHuman) {
    std::vector<std::string> dna = {"ATGC", "GCTA", "TAGC", "CGAT"};
    EXPECT_FALSE(SimianDetector::isSimian(dna));
}

// 4x4 com AAAA horizontal
TEST(SimianDetection, MinimalMatrixSimian) {
    std::vector<std::string> dna = {"AAAA", "CATG", "GCAT", "TGCA"};
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// Casos limite

// 3x3, impossivel ter sequencia de 4
TEST(SimianEdgeCases, MatrixTooSmall) {
    std::vector<std::string> dna = {"ATG", "CAG", "TTA"};
    EXPECT_FALSE(SimianDetector::isSimian(dna));
}

// Tudo A, simio com certeza
TEST(SimianEdgeCases, AllSameLetter) {
    std::vector<std::string> dna = {"AAAA", "AAAA", "AAAA", "AAAA"};
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// Aceita minuscula tambem
TEST(SimianEdgeCases, LowercaseInput) {
    std::vector<std::string> dna = {
        "aaaatg", "cagtgc", "ttatgt", "agacgg", "gcgtca", "tcactg"
    };
    EXPECT_TRUE(SimianDetector::isSimian(dna));
}

// analyze() com humano deve retornar vazio
TEST(SimianAnalysis, HumanAnalysis) {
    std::vector<std::string> dna = {
        "ATGCGA", "CAGTGC", "TTATTT", "AGACGG", "GCGTCA", "TCACTG"
    };
    SimianResult result = SimianDetector::analyze(dna);
    EXPECT_FALSE(result.isSimian);
    EXPECT_EQ(result.matches.size(), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
