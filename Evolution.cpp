#include "Evolution.h"
#include "Config.h"
#include "Utils.h"
#include <algorithm>
#include <numeric>

using namespace std;

Individuo gerarIndividuo() {
    vector<vector<double>> genoma;
    for (int i = 0; i < c.nGenes; i++) {
        vector<double> gene;
        for(int j=0; j<c.nJuntas; j++){
            // Gera velocidades iniciais aleatórias
            gene.push_back(escolherNumReal(-c.speed, c.speed));
        }
        genoma.push_back(gene);
    }
    return Individuo(genoma);
}

Individuo realizarMutacao(Individuo ind) {
    int qtdeMutados = escolherIndiceDeProbabilidades(c.listaPNumGene) + 1;
    
    vector<int> genesParaMutar;
    vector<int> indicesDisponiveis(c.nGenes); 
    iota(indicesDisponiveis.begin(), indicesDisponiveis.end(), 0);
    shuffle(indicesDisponiveis.begin(), indicesDisponiveis.end(), rng);
    
    for(int i=0; i<qtdeMutados && i < (int)indicesDisponiveis.size(); i++) {
        genesParaMutar.push_back(indicesDisponiveis[i]);
    }

    for (int idx : genesParaMutar) {
        double sinal = escolherZeroUm(c.pMutPos) ? 1.0 : -1.0;
        double mutacao = 0;

        if (c._mut == "_mut_pad") {
            mutacao = sinal * c.mutBase;
        } 
        else if (c._mut == "_mut_acu") {
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_acl") {
            if (c.incMutBase * incAtual > c.tetoMut) alterarIncrementoDaMutacaoAtual(true);
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_cao") {
            mutacao = sinal * escolherNumReal(0, 30.0); 
        }

        int r = escolherIndiceDeLista(c.nJuntas);
        ind.genoma[idx][r] += mutacao;

        if (ind.genoma[idx][r] < -c.speed) 
            ind.genoma[idx][r] = -c.speed;
        
        if (ind.genoma[idx][r] > c.speed)
            ind.genoma[idx][r] = c.speed;
    }
    return ind;
}

Individuo realizarCruzamento(const Individuo& pai1, const Individuo& pai2) {
    vector<vector<double>> novoGenoma;
    for(int i=0;i<c.nGenes;i++){
        vector<double> passoFilho;
        for(int j=0;j<c.nJuntas;j++){
            // Média simples entre os genes (velocidades) dos pais
            double gene = (pai1.genoma[i][j] + pai2.genoma[i][j]) / 2.0;
            passoFilho.push_back(gene);
        }
        novoGenoma.push_back(passoFilho);
    }
    return Individuo(novoGenoma);
}

vector<Individuo> realizarCatastrofe(vector<Individuo>& pop) {
    vector<Individuo> popNova;
    
    vector<pair<double, int>> aux;
    for(size_t i=0; i<pop.size(); i++) aux.push_back({pop[i].fitness, (int)i});
    sort(aux.rbegin(), aux.rend()); 

    int inicioZonaMorte = c.nMortosCat; 

    // Mantém a elite
    for (int i = 0; i < inicioZonaMorte; i++) 
        popNova.push_back(pop[aux[i].second]);

    // Preenche o resto com novos indivíduos aleatórios
    while (popNova.size() < pop.size()) {
        popNova.push_back(gerarIndividuo());
    }
    return popNova;
}

vector<Individuo> selecaoPorRoleta(vector<Individuo>& pop) {
    int idxPior = 0, idxMelhor = 0;
    for(size_t i=1; i<pop.size(); i++) {
        if(pop[i].fitness < pop[idxPior].fitness) idxPior = i;
        if(pop[i].fitness > pop[idxMelhor].fitness) idxMelhor = i;
    }

    vector<Individuo> novaPop;
    // Elitismo: Mantém o melhor absoluto
    novaPop.push_back(pop[idxMelhor]); 

    // Prepara a roleta
    vector<double> roleta;
    double fitnessPior = pop[idxPior].fitness;
    
    // Normalização para garantir probabilidades positivas
    // Se o fitness for negativo, desloca tudo para cima
    double normalizacao = (fitnessPior < 0) ? (-fitnessPior + 1.0) : 0.0;

    for(const auto& ind : pop) {
        roleta.push_back(ind.fitness + normalizacao + 0.1); // +0.1 para evitar zero absoluto
    }

    while(novaPop.size() < pop.size()) {
        int genitor1Idx = escolherIndiceDeProbabilidades(roleta);
        int genitor2Idx = escolherIndiceDeProbabilidades(roleta);
        
        Individuo filho = realizarCruzamento(pop[genitor1Idx], pop[genitor2Idx]);
        filho = realizarMutacao(filho);
        novaPop.push_back(filho);
    }
    return novaPop;
}