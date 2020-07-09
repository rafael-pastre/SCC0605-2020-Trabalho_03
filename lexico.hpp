#ifndef LEXICO_HPP
#define LEXICO_HPP

#include <fstream>
#include <string>

#define abrir_arq_leitura(f, arq) if ((f = fopen(arq,"r")) == NULL) { perror("Imp. ler arquivo"); exit(2); }	//Macro para abrir arquivo para leitura
#define abrir_arq_escrita(f, arq) if ((f = fopen(arq,"w")) == NULL) { perror("Imp. criar arquivo"); exit(2); }	//Macro para abrir arquivo para escrita
#define chave(est, c) ((int) c * 200 + est)

using namespace std;

void iniciar_lexico();
string obter_simbolo();
int lexico(int&, char, string&);
int transicao(int, char);

#endif
