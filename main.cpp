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

int main() {
    // Atualiza probabilidade de mutação
    c.listaPNumGene.assign(c.nGenes, 1.0/c.nGenes);
    c.listaPCadaGene.assign(c.nGenes, 1.0/c.nGenes);
    
    // Ponto alvo = gerarAlvoAleatorio();
    // cout << "Alvo Gerado -> Pos: (" << alvo.x << ", " << alvo.y << ", " << alvo.z << ")" << endl;

    Ponto alvo = {20.0, 0.0, 0.0};
    
    vector<Individuo> pop;
    for(int i=0; i<c.nIndv; i++) pop.push_back(gerarIndividuo());

    for(auto& ind : pop) calcularFitness(ind, alvo);
    melhorGeral = pop[0];

    int geracoes = 1;

    cout << "========================================\n";
    cout << "   EVOLUCAO DE TRAJETORIA ROBOTICA 3D   \n";
    cout << "========================================\n";
    cout << "Waypoints (Passos): " << c.nWaypoints << "\n";
    cout << "Total de Genes    : " << c.nGenes << "\n";
    cout << "Alvo Desejado     : (" << alvo.x << ", " << alvo.y << ", " << alvo.z << ")\n";
    cout << "Obstaculo (Bola)  : (" << bolaDeDemolicao.x << ", " << bolaDeDemolicao.y 
         << ", " << bolaDeDemolicao.z << ") Raio: " << bolaDeDemolicao.raio << "\n";
    cout << "----------------------------------------\n\n";

    while (geracoes) {

        int idxMelhorLocal = 0;
        for(size_t i=0; i<pop.size(); i++) {
            calcularFitness(pop[i], alvo);
            if(pop[i].fitness > pop[idxMelhorLocal].fitness) idxMelhorLocal = i;
        }

        Individuo& melhorLocal = pop[idxMelhorLocal];

        if (abs(melhorLocal.fitness - melhorGeral.fitness) < 0.5 && 
           (c._mut == "_mut_acu" || c._mut == "_mut_acl")) {
            estagAtual++;
            if (estagAtual > 1) alterarIncrementoDaMutacaoAtual(false);
        } 
        else if (melhorLocal.fitness > melhorGeral.fitness) {
            melhorGeral = melhorLocal;
            if (c._mut == "_mut_acu" || c._mut == "_mut_acl") alterarIncrementoDaMutacaoAtual(true);
        }

        if (estagAtual > 5) {
            if (c._mut == "_mut_acu" || c._mut == "_mut_acl") alterarIncrementoDaMutacaoAtual(true);
            pop = realizarCatastrofe(pop);
            for(auto& ind : pop) calcularFitness(ind, alvo);
        }

        if (c._sel == "_sel_rol") pop = selecaoPorRoleta(pop);
        
        // if (melhorGeral.fitness > -0.5) {
        //     cout << melhorGeral.fitness;
        //     cout << ">>> Solucao otima encontrada antes do limite de geracoes! <<<\n";
        //     break;
        // }

        geracoes++;
        
        if (geracoes % 20 == 0) {
            Ponto p = melhorGeral.trajetoria[melhorGeral.trajetoria.size() - 1];
            double dist = sqrt(pow(p.x - alvo.x, 2) + pow(p.y - alvo.y, 2) + pow(p.z - alvo.z, 2));

            cout << "Geracao " << setw(5) << geracoes 
                 << " | Fitness: " << setw(10) << fixed << setprecision(4) << melhorGeral.fitness 
                 << " | Estagnacao: " << setw(3) << estagAtual 
                 << " | Fator Mutacao: " << incAtual << "x"
                 << " | Posicao Final: (" << setw(6) << p.x << ", " 
                 << setw(6) << p.y << ", " << setw(6) << p.z << ")"
                 << " | Distancia ao Alvo: " << dist << "\n";
        }
    }

    // RELATÓRIO FINAL
    cout << "\n========================================\n";
    cout << "         RESULTADO DA SIMULACAO         \n";
    cout << "========================================\n";
    
    cout << fixed << setprecision(2);
    Ponto pZero = cinematicaDireta(poseInicial);
    cout << "[Inicio] -> Pos: (" << pZero.x << ", " << pZero.y << ", " << pZero.z << ")\n";

    for(int w = 0; w < melhorGeral.trajetoria.size(); w++) {
        vector<double> posePasso;
        
        posePasso.push_back(melhorGeral.genoma[w][0]);     
        posePasso.push_back(melhorGeral.genoma[w][1]); 
        posePasso.push_back(melhorGeral.genoma[w][2]); 

        Ponto p = melhorGeral.trajetoria[w];
        
        cout << "[Passo " << w+1 << "] -> "
             << "Pos: (" << setw(6) << p.x << ", " << setw(6) << p.y << ", " << setw(6) << p.z << ") "
             << "| Angulos: [" << setw(6) << posePasso[0] << ", " << setw(6) << posePasso[1] << ", " << setw(6) << posePasso[2] << "] " 
             << "\n";
    }

    vector<double> ultimaPose;
    ultimaPose = melhorGeral.genoma[c.nWaypoints-1];
    Ponto finalReal = cinematicaDireta(ultimaPose);
    
    double distFinal = sqrt(pow(finalReal.x - alvo.x, 2) + pow(finalReal.y - alvo.y, 2) + pow(finalReal.z - alvo.z, 2));

    cout << "----------------------------------------\n";
    cout << "Distancia Final ao Alvo: " << distFinal << "\n";
    cout << "Fitness Total          : " << melhorGeral.fitness << "\n";
    cout << "========================================\n";

    return 0;
}