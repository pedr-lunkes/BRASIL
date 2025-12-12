#include "Evolution.h"
#include "Config.h"
#include "Utils.h"
#include <algorithm>
#include <numeric>

using namespace std;

/// @brief Cria um novo indivíduo com genoma inicializado aleatoriamente.
/// 
/// Constrói a matriz genética de dimensão [nGenes x nJuntas], onde cada gene
/// representa uma velocidade angular. Os valores são sorteados uniformemente 
/// dentro dos limites de velocidade definidos em Config.
/// 
/// @return Um objeto do tipo 'Individuo' contendo a matriz de velocidades gerada.
Individuo gerarIndividuo() {
    vector<vector<double>> genoma;
    for (int i = 0; i < c.nGenes; i++) {
        vector<double> gene;
        for(int j=0; j<c.nJuntas; j++){
            gene.push_back(escolherNumReal(-c.speed, c.speed));
        }
        genoma.push_back(gene);
    }
    return Individuo(genoma);
}

/// @brief Aplica a mutação adaptativa em um indivíduo.
/// 
/// 1. Define quantos e quais genes serão alterados.
/// 2. Calcula a magnitude da mutação baseada na estagnação atual (incAtual), permitindo
///    saltos maiores se o algoritmo estiver preso (lógica adaptativa).
/// 3. Aplica a alteração na velocidade de uma junta aleatória garantindo que a velocidade não exceda o limite físico.
/// 
/// @param ind O indivíduo original a ser mutado.
/// @return Uma cópia do indivíduo com as modificações aplicadas. 
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

        mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);

        int r = escolherIndiceDeLista(c.nJuntas);
        ind.genoma[idx][r] += mutacao;

        if (ind.genoma[idx][r] < -c.speed) 
            ind.genoma[idx][r] = -c.speed;
        
        if (ind.genoma[idx][r] > c.speed)
            ind.genoma[idx][r] = c.speed;
    }
    return ind;
}

/// @brief Realiza o Crossover Aritmético entre dois indivíduos.
///
/// O novo indivíduo herda a média simples dos genes (velocidades) dos pais.
///
/// @param pai1 
/// @param pai2 
/// @return Novo indivíduo resultante do cruzamento.
Individuo realizarCruzamento(const Individuo& pai1, const Individuo& pai2) {
    vector<vector<double>> novoGenoma;
    for(int i=0;i<c.nGenes;i++){
        vector<double> passoFilho;
        for(int j=0;j<c.nJuntas;j++){
            double gene = (pai1.genoma[i][j] + pai2.genoma[i][j]) / 2.0;
            passoFilho.push_back(gene);
        }
        novoGenoma.push_back(passoFilho);
    }
    return Individuo(novoGenoma);
}

/// @brief Aplica uma catástrofe na população atual.
///
/// 1.Ordena a população inteira por fitness (do melhor para o pior).
/// 2.Preserva uma pequena elite (os sobreviventes) para garantir que a melhor solução não se perca.
/// 3.Substitui todo o restante da população por novos indivíduos totalmente aleatórios.
///
/// @param pop A população atual que se encontra em estagnação.
/// @return Nova população composta pela elite sobrevivente e novos indivíduos aleatórios.
vector<Individuo> realizarCatastrofe(vector<Individuo>& pop) {
    vector<Individuo> popNova;
    
    // Ordena por fitness
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

/// @brief Gera a próxima geração de indivíduos aplicando Seleção por Roleta.
/// 
///  Reprodução: Seleciona pais via Roleta (maior fitness = maior chance), realiza o 
///  Cruzamento Aritmético e aplica a Mutação no filho gerado.
/// 
/// @param pop A população da geração atual.
/// @return A nova população evoluída (mesmo tamanho da anterior).
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
    double normalizacao = (fitnessPior < 0) ? (-fitnessPior + 1.0) : 0.0;

    for(const auto& ind : pop) {
        roleta.push_back(ind.fitness + normalizacao + 0.1); // +0.1 para evitar zero absoluto
    }

    // Geração da nova população
    while(novaPop.size() < pop.size()) {
        int genitor1Idx = escolherIndiceDeProbabilidades(roleta);
        int genitor2Idx = escolherIndiceDeProbabilidades(roleta);
        
        Individuo filho = realizarCruzamento(pop[genitor1Idx], pop[genitor2Idx]);
        filho = realizarMutacao(filho);
        novaPop.push_back(filho);
    }
    return novaPop;
}
