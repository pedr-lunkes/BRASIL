#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

#include "Types.h"
#include "Config.h"
#include "Utils.h"
#include "Robot.h"
#include "Evolution.h"

using namespace std;

// Variável para armazenar o melhor global
Individuo melhorGeral;

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

// ---- Funções para a comunicação com o script em python ----
// Função que manda a trajetoria do melhor indivíduo no terminal
void imprimirTrajetoria() {
    cout << "START_PATH" << endl;
    for(int i = 0; i < melhorGeral.trajetoria.size(); i++) {
        Ponto p = melhorGeral.trajetoria[i];
        cout << p.x << " " << p.y << " " << p.z << endl;
    }
    cout << "END_PATH" << endl;
    cout.flush();
}

// Função que manda algumas estatísticas do Algoritmo Evolutivo para plot
void imprimirEstatisticas(int geracao, double mediaFit) {
    // Formato: STATS <geracao> <melhor_fit> <media_fit> <tamanho_trajetoria>
    cout << "STATS " 
         << geracao << " " 
         << melhorGeral.fitness << " " 
         << mediaFit << " " 
         << melhorGeral.trajetoria.size() << endl;
}

int main(int argc, char* argv[]) {
    // Leitura dos argumentos
    double tx = 20.0, ty = 0.0, tz = 0.0;
    if (argc >= 4) {
        tx = atof(argv[1]);
        ty = atof(argv[2]);
        tz = atof(argv[3]);
    }

    Ponto alvo = {tx, ty, tz};

    // Configurações
    c.listaPNumGene.assign(c.nGenes, 1.0/c.nGenes);
    c.listaPCadaGene.assign(c.nGenes, 1.0/c.nGenes);

    // Inicialização
    vector<Individuo> pop;
    for(int i=0; i<c.nIndv; i++) pop.push_back(gerarIndividuo());

    for(auto& ind : pop) calcularFitness(ind, alvo);
    melhorGeral = pop[0];

    int geracoes = 0;
    
    // Loop Infinito: O programa roda até o Python matar o processo
    while (true) {
        int idxMelhorLocal = 0;
        double somaFitness = 0.0;

        for(size_t i=0; i<pop.size(); i++) {
            calcularFitness(pop[i], alvo);
            somaFitness += pop[i].fitness;
            if(pop[i].fitness > pop[idxMelhorLocal].fitness) idxMelhorLocal = i;
        }

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

        // Evolução
        if (estagAtual > c.minEstagCat) {
            alterarIncrementoDaMutacaoAtual(true);
            pop = realizarCatastrofe(pop);
            for(auto& ind : pop) calcularFitness(ind, alvo);
        }

        pop = selecaoPorRoleta(pop);
        geracoes++;
    }

    return 0;
}