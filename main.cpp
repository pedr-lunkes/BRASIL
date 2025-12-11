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

// Função auxiliar para imprimir a trajetória atual
void imprimirTrajetoria() {
    // Marcador de inicio para o Python identificar
    cout << "START_PATH" << endl;

    for(int i = 0; i < melhorGeral.trajetoria.size(); i++) {
        Ponto p = melhorGeral.trajetoria[i];
        cout << p.x << " " << p.y << " " << p.z << endl;
    }
    
    // Marcador de fim
    cout << "END_PATH" << endl;
    
    // Flush para garantir que o Python receba imediatamente
    cout.flush();
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
        for(size_t i=0; i<pop.size(); i++) {
            calcularFitness(pop[i], alvo);
            if(pop[i].fitness > pop[idxMelhorLocal].fitness) idxMelhorLocal = i;
        }

        Individuo& melhorLocal = pop[idxMelhorLocal];

        if (melhorLocal.fitness > melhorGeral.fitness) {
            melhorGeral = melhorLocal;
            alterarIncrementoDaMutacaoAtual(true);
        } else if(abs(melhorLocal.fitness - melhorGeral.fitness) < 0.5){
            estagAtual++;
            if (estagAtual > 1) alterarIncrementoDaMutacaoAtual(false);
        }

        // --- STREAMING DE DADOS ---
        // A cada 200 gerações (ou na primeira), envia a melhor rota atual
        if (geracoes % 3 == 0) {
            imprimirTrajetoria();
        }

        // Evolução
        if (estagAtual > 5) {
            alterarIncrementoDaMutacaoAtual(true);
            pop = realizarCatastrofe(pop);
            for(auto& ind : pop) calcularFitness(ind, alvo);
        }

        pop = selecaoPorRoleta(pop);
        
        geracoes++;
    }

    return 0;
}