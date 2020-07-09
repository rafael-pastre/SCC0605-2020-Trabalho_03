#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cctype>

#include "lexico.hpp"

//Ativa/Desativa mensagens de debug
//#define LEX_DBG

//Hash maps para tabelas
unordered_map<string, string> reservados;
unordered_map<int, int> transicoes;

//Variáveis globais do sintático
extern ifstream ler;
extern string id;
extern string val;
extern int linha;

//Inicializa analisador léxico
void iniciar_lexico() {
	FILE* fp;
	linha = 1;
	
	//Cria e carrega tabela hash das palavras e símbolos reservados
	abrir_arq_leitura(fp, "reservados.txt");
	char p[256], t[256];
	string palavra, token;
	while ( !feof(fp) ) {
		fscanf(fp, "%s %s", p, t);
		palavra = p; token = t;
		reservados.insert(pair<string, string> (palavra, token));
	}
	fclose(fp);
	
	//Cria e carrega tabela hash com transições de entados
	abrir_arq_leitura(fp, "transicoes.txt");
	int atual, prox;
	char c;
	while ( !feof(fp) ) {
		fscanf(fp, "%d %c %d", &atual, &c, &prox);
		transicoes.insert(pair<int, int> (chave(atual, c), prox));
	}
	fclose(fp);
}

string obter_simbolo() {
	static int ret = 0;		//indica se há retorno do analisador léxico (ret != 0) e se é necessário retroceder na leitura (ret < 0)
	static char c;			//caractere de entrada
	int s = 0;				//estado do analisador léxico
	string simbolo;
	
	if (ret >= 0) ler.get(c);

	while (!ler.eof()) {
		if (ret >= 0 && c == '\n') linha++;		//Conta linha

		ret = lexico(s, c, simbolo);
			
		if (ret != 0) {
			#ifdef LEX_DBG 
			cout << simbolo << endl;
			#endif
			
			//Despreza comentários
			if (simbolo.compare("COMENTARIO") != 0) 
				return simbolo;
			else
				s = 0;
		}
		
		if (ret >= 0) ler.get(c);
	}
}

/*	lê entrada e altera estado. Se houver saída escreve nela 
*	retorno indica se há saída. 
		retorno = 0 -> sem saída
		retorno > 0 -> com saída
		retorno < 0 -> saída com retorceder
*/
int lexico(int& est, char entrada, string& saida) {
	static int cnt;				//conta posição onde está a cadeia
	static char cadeia[256];	//cadeia sendo lida
	string token;
	
	if (est == 0) cnt = 0;		//Limpa cadeia no estado inicial
	
	//Transição de estados
	est = transicao(est, entrada);
	#ifdef LEX_DBG 
	cout << est << " ";
	#endif

	cadeia[cnt++] = entrada;	//Guarda cadeia sendo lida
	cadeia[cnt] = '\0';			//Fim da string será sobrescrito caso não o seja
	
	//Executa ações para estados que as possuem
	switch (est) {
		case 2:
			saida =  "simb_atrib";
			return 1;
			
		case 3:
			saida =  "simb_dp";
			return -1;

		case 5:
			saida =  "simb_menor_igual";
			return 1;

		case 6:
			saida =  "simb_dif";
			return 1;

		case 7:
			saida =  "simb_menor";
			return -1;

		case 9:
			saida =  "simb_maior_igual";
			return 1;

		case 10:
			saida =  "simb_maior";
			return -1;

		case 12:
			cadeia[--cnt] = '\0'; 			//Retira o caractere 'outro' da cadeia
			token = reservados[cadeia];		//Realiza busca na tabela de palavras reservadas
			
			if (!token.empty()) {
				saida =  token;
			} else {
				id = cadeia;
				saida =  "id";
			}
			return -1;

		case 14:
			saida =  "COMENTARIO";
			return 1;

		case 18:
			cadeia[--cnt] = '\0'; 			//Retira o caractere 'outro' da cadeia
			val = cadeia;
			saida =  "NUM_REAL";
			return -1;

		case 19:
			cadeia[--cnt] = '\0'; 			//Retira o caractere 'outro' da cadeia
			val = cadeia;
			saida =  "NUM_INT";
			return -1;

		case 20:
			saida =  strcat(cadeia, ", NUM_REAL_MAL_FORMADO");
			return -1;
		
		case 21:
			token = reservados[cadeia];		//Realiza busca na tabela de símbolos reservadas
			
			if (!token.empty()) {
				saida =  token;
			} else {
				saida =  strcat(cadeia, ", CARACTERE_INVALIDO");
			}
			return 1;
		
		default:
			return 0;
	}
}

/* Função para transição de estados
*  recebe estado atual e caractere e retorna prox. estado
*/
int transicao(int atual, char c) {	
	//'Normaliza' letras e números
	if (isdigit(c)) c = '0';
	if (isalpha(c)) c = 'a';
	
	//Analisa caracteres não carregáveis pela tabela
	if (atual == 0 && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) return 0;

	//Realiza busca na tabela de transições
	int h = transicoes[chave(atual, c)];
	
	//Explora opção 'outro' usando '?'
	if (h == 0) h = transicoes[chave(atual, '?')];
	
	return h;
}

