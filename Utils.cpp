#include "Utils.h"

// Definição das variáveis globais
std::mt19937 rng(std::random_device{}());
int estagAtual = 0;
double incAtual = 0.0;

// Altera o incremento da mutação baseado na estagnação
void alterarIncrementoDaMutacaoAtual(bool resetar) {
    if (resetar){ 
        incAtual = 0;
        estagAtual = 0;
    }
    else incAtual++;
}

// Escolhe um número real aleatório entre min e max
double escolherNumReal(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

// Escolhe 'true' com probabilidade 'prob', 'false' caso contrário
bool escolherZeroUm(double prob) {
    std::bernoulli_distribution d(prob);
    return d(rng);
}

// Escolhe um índice baseado em uma lista de probabilidades
int escolherIndiceDeProbabilidades(const std::vector<double>& probs) {
    std::discrete_distribution<int> d(probs.begin(), probs.end());
    return d(rng);
}

// Escolhe um índice aleatório entre 0 e size-1
int escolherIndiceDeLista(int size) {
    std::uniform_int_distribution<int> d(0, size - 1);
    return d(rng);
}