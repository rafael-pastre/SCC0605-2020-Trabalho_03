#ifndef SEMANTICO_HPP
#define SEMANTICO_HPP

#include <string>
#include <vector>

using namespace std;

//Elemento da tabela de símbolos
typedef struct {
	string token;
	string categoria;
	string tipo;
	unsigned int endereco;
	union  {
		float v_real;
		int v_inteiro;
	};
} elemento;

void criar_elemento(elemento& elmnt, string token, string categoria);

#endif
