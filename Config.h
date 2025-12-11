#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
using namespace std;

struct Config {
    int nIndv = 100;           
    int nMortosCat = 30;       
    double pCat = 0.05;        
    
    int nWaypoints = 100;      
    int nJuntas = 3;
    int nGenes; // Calculado no construtor
    
    // Limites
    vector<double> baseLmin = {-180.0, 0.0, 0.0};
    vector<double> baseLmax = {180.0, 90.0, 180.0};
    double speed = 5;
    
    // Mutação
    string _mut = "_mut_acu"; 
    double pMutPos = 0.5;          
    double mutBase = 1.0;          
    double incMutBase = 0.5;       
    double tetoMut = 10.0;         
    
    // Catástrofe e Seleção
    string _cat = "_cat_dis"; 
    string _sel = "_sel_rol"; 

    // Probabilidades
    vector<double> listaPNumGene; 
    vector<double> listaPCadaGene;

    Config(); // Construtor
};

// Declaração da variável global 'c' (definida no .cpp)
extern Config c;

#endif