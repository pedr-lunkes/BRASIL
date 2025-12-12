#include <iostream>
#include <vector>
#include <cmath>

#include "Types.h"
#include "Config.h"
#include "Utils.h"
#include "Robot.h"
#include "Evolution.h"

using namespace std;

// Variável para armazenar o melhor global
Individuo melhorGeral;

/// @brief Gera um ponto alvo aleatório que seja fisicamente alcançável pelo robô.
/// 
/// Sorteia ângulos aleatórios para cada junta (respeitando os limites min/max definidos no Config.h)
/// e converte esses ângulos para uma coordenada cartesiana (X, Y, Z) usando a Cinemática Direta.
///
/// @return Um objeto do tipo 'Ponto' contendo as coordenadas do alvo gerado.
Ponto gerarAlvoAleatorio() {
    vector<double> angulosValidos;
    
    // Gera um ângulo aleatório para cada junta respeitando os limites do Config.h
    for (int i = 0; i < c.nJuntas; i++) {
        double angulo = escolherNumReal(c.baseLmin[i], c.baseLmax[i]);
        angulosValidos.push_back(angulo);
    }

    Ponto alvoGerado = cinematicaDireta(angulosValidos);

    return alvoGerado;
}

/// @brief Imprime a trajetória do melhor indivíduo no formato esperado pelo script Python.
void imprimirTrajetoria() {
    cout << "START_PATH" << endl;
    for(int i = 0; i < melhorGeral.trajetoria.size(); i++) {
        Ponto p = melhorGeral.trajetoria[i];
        cout << p.x << " " << p.y << " " << p.z << endl;
    }
    cout << "END_PATH" << endl;
    cout.flush();
}

/// @brief Manda estatísticas da geração atual para o script Python.
/// @param geracao Número da geração atual
/// @param mediaFit Média de fitness da população na geração atual
void imprimirEstatisticas(int geracao, double mediaFit) {
    // Formato: STATS <geracao> <melhor_fit> <media_fit> <tamanho_trajetoria>
    cout << "STATS " 
         << geracao << " " 
         << melhorGeral.fitness << " " 
         << mediaFit << " " 
         << melhorGeral.trajetoria.size() << endl;
}

/// @brief Imprime as informações do obstáculo no formato esperado pelo script Python.
void imprimirObstaculo() {
    cout << "OBSTACLE "
         << c.bolaDeDemolicao.x << " "
         << c.bolaDeDemolicao.y << " "
         << c.bolaDeDemolicao.z << " "
         << c.bolaDeDemolicao.raio << endl;
}

int main(int argc, char* argv[]) {
    double tx = 20.0, ty = 0.0, tz = 0.0;
    if (argc >= 4) {
        tx = atof(argv[1]);
        ty = atof(argv[2]);
        tz = atof(argv[3]);
    }

    // Gera o ponto alvo
    Ponto alvo = {tx, ty, tz};

    // Configurações e inicializações
    c.listaPNumGene.assign(c.nGenes, 1.0/c.nGenes);
    c.listaPCadaGene.assign(c.nGenes, 1.0/c.nGenes);
    vector<Individuo> pop;
    for(int i=0; i<c.nIndv; i++) pop.push_back(gerarIndividuo());

    for(auto& ind : pop) calcularFitness(ind, alvo);
    melhorGeral = pop[0];
    int geracoes = 0;
    imprimirObstaculo();
    
    // Loop Infinito: O programa roda até o Python matar o processo
    while (true) {
        int idxMelhorLocal = 0;
        double somaFitness = 0.0;

        // Avaliação da população
        for(size_t i=0; i<pop.size(); i++) {
            calcularFitness(pop[i], alvo);
            somaFitness += pop[i].fitness;
            if(pop[i].fitness > pop[idxMelhorLocal].fitness) idxMelhorLocal = i;
        }

        // Atualiza melhor local e global
        Individuo& melhorLocal = pop[idxMelhorLocal];
        double mediaFitness = somaFitness/c.nIndv;

        if (melhorLocal.fitness > melhorGeral.fitness) {
            melhorGeral = melhorLocal;
            alterarIncrementoDaMutacaoAtual(true);
        } else if(abs(melhorLocal.fitness - melhorGeral.fitness) < 0.5){
            estagAtual++;
            if (estagAtual > c.minEstag) alterarIncrementoDaMutacaoAtual(false);
        }

        // Streaming de dados
        if (geracoes % c.printGeracoes == 0) {
            imprimirTrajetoria();
            imprimirEstatisticas(geracoes, mediaFitness);
        }

        // Catastrófe
        if (estagAtual > c.minEstagCat) {
            alterarIncrementoDaMutacaoAtual(true);
            pop = realizarCatastrofe(pop);
            for(auto& ind : pop) calcularFitness(ind, alvo);
        }

        // Seleção e Mutação
        pop = selecaoPorRoleta(pop);
        geracoes++;
    }

    return 0;
}
