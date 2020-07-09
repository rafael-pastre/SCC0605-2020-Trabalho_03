#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "lexico.hpp"
#include "semantico.hpp"

// Change this below to disable/enable debug messages
#define dprint(message)
//#define dprint(message) cout << linha << " " << #message << endl;

#define key(ident) (escopo + "_" + ident)	//Chave da tabela de símbolos
#define gerar_cod(codigo) do { if (!num_erros) {cod.push_back(codigo); lin_cod++; } } while(0)		//Gera codigo - do while permite usar ';' na chamada

#define IMPR_LINHAS		//Definir para imprimir linhas no código de saída
//#define IMPR_TABELA	//Definir para imprimir tabela de símbolos
	
using namespace std;

void programa();
void dc_c(vector<string>);
void dc_v(vector<string>, vector<string>);
string tipo_var(vector<string>, vector<string>);
vector<string> variaveis(vector<string>, vector<string>);
void dc_p(vector<string>);
void corpo_p(vector<string>, vector<string>);
void argumentos(vector<string>, vector<string>, vector<string>);
void comandos(vector<string>, vector<string>);
void cmd(vector<string>, vector<string>);
void condicao(vector<string>, vector<string>);
string expressao(vector<string>, vector<string>);
string termo(vector<string>);

void erro();
void erro(string);
int  erro(string, vector<string>);
int  erro(string, vector<string>, vector<string>);
void erro_sem(string);

unordered_map<string, vector<string>> param;	//Armazena parâmetros de procedimentos
unordered_map<string, elemento> tabela;			//Tabela de Símbolos

vector<string> cod;			//Cada elemento armazena um código gerado
int lin_cod;				//Linha do código gerado
int end_dados;				//Endereço da pilha de dados
int lin_dsvi_principal;		//Armazena endereço do programa principal

string escopo;				//Armazena escopo
string escopo_principal;	//Armazena escopo principal

ifstream ler;
ofstream saida;
string simb;	 	//Guarda símbolo lido
string id;		 	//Guarda nome do identificador
string val;			//Guarda valor dos nums. int e float
int linha;			//Conta linha
int num_erros = 0;	//Conta qtde de erros

//Guarda comandos (muito utilizado para sincronização no erro)
vector<string>cmds = {"simb_begin", "simb_read", "simb_write", "simb_for", "simb_if", "simb_while", "id"};

int main(int argc, char** argv) {
	
	if (argc != 3) {
		perror("Parametros incorretos");
		exit(1);
	}
	
	//Abre arquivos de leitura e saída do analisador sintático 
	ler.open(argv[1], ios::in);
	saida.open(argv[2], ios::out);
	
	if (!ler.is_open() || !saida.is_open()) {
		cout << "Impossivel abrir/criar arquivo(s)" << endl;
		exit(0);
	}
	
	iniciar_lexico();
	
	//Inicia análise sintática
	lin_cod = 0;
	cout << endl;
	simb = obter_simbolo();
	programa();
	if (!ler.eof()) {
		erro("Sem comandos depois do fim do programa");
	}
	cout << endl;
	cout << num_erros << " erros detectados" << endl;
	cout << "Analise finalizada" << endl;
		
	if (num_erros == 0) {
		cout << endl;
		cout << "Compilacao bem-sucedida" << endl;
		
		//Escreve codigo no arq. de saída
		for (int i = 0; i < cod.size(); i++) {
			#ifdef IMPR_LINHAS
			saida << i << ") ";
			#endif
			
			saida << cod[i] << endl;
		}
	}
	
	ler.close();
	saida.close();
	
	//Imprime tabela de símbolos
	#ifdef IMPR_TABELA
	for ( auto local_it = tabela.begin(); local_it != tabela.end(); ++local_it )
    	cout << " " << local_it->first << ":" << local_it->second.categoria << " " << local_it->second.tipo << endl;
	cout << endl;
	#endif
	
	return 0;
}

void programa() {
	lin_dsvi_principal = 0;
	
	if (simb.compare("simb_program") == 0) {
		dprint(Em: <programa>: simb_program lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("program", {"id"});
	}
	
	if (simb.compare("id") == 0) {
		dprint(Em: <programa>: id lido pelo sintatico)
		
		escopo = id;			//Altera escopo
		escopo_principal = id;
		
		elemento prog;  criar_elemento(prog, "id", "nome_program");
		tabela[key(id)] = prog;
		simb = obter_simbolo();
	} else {
		erro("identificador", {"simb_pv"});
	}
	
	if (simb.compare("simb_pv") == 0) {
		dprint(Em: <programa>: simb_pv lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("\';\'", {"simb_const", "simb_var", "simb_procedure", "simb_begin"});
	}
	
	gerar_cod("INPP");
	
	dc_c({"simb_begin", "simb_var", "simb_procedure"});
	dc_v({"simb_begin", "simb_procedure"}, {});
	dc_p({"simb_begin"});
	
	if (!num_erros && lin_dsvi_principal != 0) cod[lin_dsvi_principal] += to_string(lin_cod);	//Completa end. de desvio para o programa principal
	
	if (simb.compare("simb_begin") == 0) {
		dprint(Em: <programa>: simb_begin lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("begin", {"simb_end"});
	}
	
	comandos({"simb_end"}, {});
	
	if (simb.compare("simb_end") == 0) {
		dprint(Em: <programa>: simb_end lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("end", {"simb_p"});
	}
	
	if (simb.compare("simb_p") == 0) {
		dprint(Em: <programa>: simb_p lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("\'.\'");
	}
	
	gerar_cod("PARA");
}

void dc_c(vector<string> S){
	
	// const
	while(simb.compare("simb_const") == 0){
		int ret;
		dprint(Em: <dc_c>: simb_const lido pelo sintatico)
		simb = obter_simbolo();
				
		// ident
		if (simb.compare("id") == 0) {
			dprint(Em: <dc_c>: id lido pelo sintatico)
			
			if (tabela.count(key(id)) == 0) {		//Insere na tabela
				elemento cte;
				criar_elemento(cte, simb, "const");
				tabela[key(id)] = cte;
			} else {
				erro_sem("Identificador declarado novamente");
			}
			
			simb = obter_simbolo();
		} else {
			ret = erro("identificador", {"simb_igual", "simb_integer", "simb_real"}, S);
			if (ret <= 0) return;
		}
		
		// =
		if (simb.compare("simb_igual") == 0) {
			dprint(Em: <dc_c>: simb_igual lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\'=\'", {"simb_integer", "simb_real"}, S);
			if (ret <= 0) return;
		}
		
		// <numero>
		if (simb.compare("NUM_REAL") == 0 || simb.compare("NUM_INT") == 0) {
			dprint(Em: <dc_c>: <numero> lido pelo sintatico)
			
			//Define tipo e valor da constante
			elemento& cte = tabela[key(id)];
			if (simb.compare("NUM_REAL") == 0) {
				cte.tipo = "real";
				cte.v_real = stof(val);
			} else if (simb.compare("NUM_INT") == 0) {
				cte.tipo = "inteiro";
				cte.v_inteiro = stoi(val);
			}
			
			simb = obter_simbolo();
		} else {
			ret = erro("Numero", {"simb_pv"}, S);
			if (ret <= 0) return;
		}
		
		// ;
		if (simb.compare("simb_pv") == 0) {
			dprint(Em: <dc_c>: simb_pv lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\';\'", {"simb_const"}, S);
			if (ret <= 0) return;
		}
	}
}

void dc_v(vector<string> local, vector<string> S){
	int ret;
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	vector<string> idents;	//Guarda identificadores para atrib. tipo
	string var, tipo;
	
	// var
	while(simb.compare("simb_var") == 0){
		dprint(Em: <dc_v>: simb_var lido pelo sintatico)
		simb = obter_simbolo();
		
		// <variaveis>
		idents = variaveis({"simb_procedure", "simb_begin"}, S);
		
		// :
		if (simb.compare("simb_dp") == 0) {
			dprint(Em: <dc_v>: simb_dp lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\':\'", {"simb_real", "simb_integer", "simb_pv"}, S);
			if (ret <= 0) return;
		}
		
		// <tipo_var>
		tipo = tipo_var({"simb_procedure", "simb_begin"}, S);
		
		//Adiciona tipo às variáveis e gera código
		while (!idents.empty()) {
			var = idents.back();
			idents.pop_back();
			tabela[key(var)].tipo = tipo;
			
			//Ordem de alocação inversa a de declaração
			tabela[key(var)].endereco = end_dados++;
			gerar_cod("ALME 1");
		}
		
		// ;
		if (simb.compare("simb_pv") == 0) {
			dprint(Em: <dc_v>: simb_pv lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\';\'", {"simb_var", "simb_procedure", "simb_begin"}, S);
			if (ret <= 0) return;
		}
	}
}

//Retorna tipo
string tipo_var(vector<string> local, vector<string> S){
	int ret;
	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	
	// "real" ou "integer"
	if (simb.compare("simb_real") == 0) {
		dprint(Em: <tipo_var>: simb_real lido pelo sintatico)
		simb = obter_simbolo();
		return "real";
	} else if(simb.compare("simb_integer") == 0){
		dprint(Em: <tipo_var>: simb_integer lido pelo sintatico)
		simb = obter_simbolo();
		return "inteiro";
	} else {
		ret = erro("Tipo", V);
		if (ret <= 0) return "";
	}
}

//Retorna variáveis declaradas
vector<string> variaveis(vector<string> local, vector<string> S){
	int continua, ret;
	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	vector<string> idents; 		//Guarda identificadores declarados

	do {
		continua = 0;
		// ident
		if (simb.compare("id") == 0) {
			dprint(Em: <variaveis>: id lido pelo sintatico)
			
			if (tabela.count(key(id)) == 0) {		//Insere na tabela
				elemento var;
				criar_elemento(var, simb, "var");
				tabela[key(id)] = var;
				idents.push_back(id);
			} else {
				erro_sem("Identificador declarado novamente");
			}
			
			simb = obter_simbolo();
		} else {
			ret = erro("identificador", {"simb_v", "id"}, V);
			if (ret <= 0) return idents;
		}
		
		// ',' ou 'lambda'
		if (simb.compare("simb_v") == 0) {
			dprint(Em: <variaveis>: simb_v lido pelo sintatico)
			simb = obter_simbolo();
			continua = 1;
		}
	} while(continua);
}

void dc_p(vector<string> S){
	int ret, n_desaloc, tam;
	string tipo, proc_name, par;
	vector<string> idents;
	
	if (simb.compare("simb_procedure") == 0) {
		lin_dsvi_principal = lin_cod;	//Salva linha do desvio
		gerar_cod("DSVI ");				//Necessário completar depois com end. do programa principal
	}
	
	// "procedure" ou 'lambda'
	while (simb.compare("simb_procedure") == 0) {
		dprint(Em: <dc_p>: simb_procedure lido pelo sintatico)
		simb = obter_simbolo();
		
		// ident
		if (simb.compare("id") == 0) {
			dprint(Em: <dc_p>: id lido pelo sintatico)
			
					
			if (tabela.count(key(id)) == 0) {			//Insere na tabela
				elemento proc;
				criar_elemento(proc, simb, "proc");
				tabela[key(id)] = proc;
				tabela[key(id)].endereco = lin_cod;		//Salva endereço do código
				proc_name = id;
				escopo = id;							//Altera escopo para salvar na tabela
			} else {
				erro_sem("Procedimento declarado novamente");
			}
			
			simb = obter_simbolo();
		} else {
			ret = erro("identificador", {"simb_apar", "id", "simb_dp"}, S);
			if (ret <= 0) return;
		}
		
		end_dados++;		//Considera endereço de retorno
		n_desaloc = 0;		//Guarda num. de var./param. para desalocar
		
		// '(' ou 'lambda'
		if (simb.compare("simb_apar") == 0) {
			dprint(Em: <dc_p>: simb_apar lido pelo sintatico)
			simb = obter_simbolo();
			
			while(1){
				// <variaveis>
				idents = variaveis({}, {});
				tam = idents.size();
				n_desaloc += tam;
				
				// :
				if (simb.compare("simb_dp") == 0) {
					dprint(Em: <dc_p>: simb_dp lido pelo sintatico)
					simb = obter_simbolo();
				} else {
					erro("\':\'");
				}
			
				// <tipo_var>
				tipo = tipo_var({}, {});
				
				//Copia parâmetros para a tabela de símbolos
				//Muda categoria para parâmetros e adiciona tipo e endereço
				end_dados += tam;
				while (!idents.empty()) {
					par = idents.back();
					idents.pop_back();
					tabela[key(par)].tipo = tipo;
					tabela[key(par)].categoria = "par";
					if(!proc_name.empty()) param[proc_name].push_back(tipo); 	//Salva tipos dos parâmetros do procedimento
					
					tabela[key(par)].endereco = --end_dados;					//Endereços dos parâmetros
					gerar_cod("COPVL");
				}
				end_dados += tam;		//Rearranja endereço de dados, pois os parâmetros foram percorridos de trás para frente

				
				// ';' ou ')'
				if (simb.compare("simb_pv") == 0) {
					dprint(Em: <dc_p>: simb_pv lido pelo sintatico)
					simb = obter_simbolo();
					continue;
				} else if(simb.compare("simb_fpar") == 0){
					dprint(Em: <dc_p>: simb_fpar lido pelo sintatico)
					simb = obter_simbolo();
					break;
				} else {
					erro();
					break;
				}
			}
			
		}
		
		// ;
		if (simb.compare("simb_pv") == 0) {
			dprint(Em: <dc_p>: simb_pv lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\';\'", {"simb_var", "simb_begin"}, S);
			if (ret <= 0) return;
		}
		
		// <corpo_p>
		int end_dados_ant = end_dados;
		corpo_p({"simb_procedure", "simb_begin"}, S);
		
		n_desaloc += (end_dados - end_dados_ant);	//Soma qtde de variáveis criadas no corpo_p para desalocar
		gerar_cod("DESM " + to_string(n_desaloc));	//Desaloca parâmetros + variáveis declaradas no procedimento
		
		//Retira da pilha de dados posições desalocadas e endereço de retorno
		end_dados = end_dados - n_desaloc - 1;
		
		gerar_cod("RTPR");
		
		escopo = escopo_principal;	//Retorna ao escopo principal (Torna inacessível os parâmetros e variéveis do procedimento)
	}
	
}

void corpo_p(vector<string> local, vector<string> S){
	int ret;
	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));

	// <dc_v>
	dc_v(cmds, V);
	
	// begin
	if (simb.compare("simb_begin") == 0) {
		dprint(Em: <corpo_p>: simb_begin lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		ret = erro("begin", cmds, V);
		if (ret <= 0) return;
	}
	
	// <comandos>
	comandos({"simb_pv"}, V);
	
	// end
	if (simb.compare("simb_end") == 0) {
		dprint(Em: <corpo_p>: simb_end lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		ret = erro("end", {"simb_pv"}, V);
		if (ret <= 0) return;
	}
	
	// ;
	if (simb.compare("simb_pv") == 0) {
		dprint(Em: <corpo_p>: simb_pv lido pelo sintatico)
		simb = obter_simbolo();
	} else {
		erro("\';\'");
	}
}

void argumentos(vector<string> local, vector<string> S, vector<string> param_proc){
	int ret, i;
	string tipo_param;
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));

	i  = 0;
	while (simb.compare("id") == 0) {
		// ident
		dprint(Em: <argumentos>: id lido pelo sintatico)
		
		if (tabela.count(key(id)) == 0) {		//Busca na tabela
			erro_sem("Identificador nao declarado");
		} else {
			//Checar parâmetros param_proc
			if (i >= param_proc.size()) erro_sem("Numero excessivo de argumentos");
			else {
				tipo_param = param_proc[i++];
				if (tipo_param.compare("inteiro") == 0 && tabela[key(id)].tipo.compare("real") == 0) erro_sem("Parametros incompativeis");	//Aceita tipos iguais e real<=inteiro, mas não inteiro<=real
				
				gerar_cod("PARAM " + to_string(tabela[key(id)].endereco));
			}
		}
		simb = obter_simbolo();
				
		// ';' ou 'lambda'
		if (simb.compare("simb_pv") == 0) {
			dprint(Em: <argumentos>: simb_pv lido pelo sintatico)
			simb = obter_simbolo();
		}
	}
	if (i < param_proc.size()) erro_sem("Numero insuficiente de argumentos");
}

void comandos(vector<string> local, vector<string> S){
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	while(	simb.compare("simb_read")	== 0 ||
			simb.compare("simb_write")	== 0 ||
			simb.compare("simb_while")	== 0 ||
			simb.compare("simb_if")		== 0 ||
			simb.compare("id")			== 0 ||
			simb.compare("simb_begin")	== 0 ||
			simb.compare("simb_for")	== 0 )
	{
		dprint(Em: <comandos>: comando valido lido pelo sintatico)		
		cmd({"simb_pv"}, S);
		
		if (simb.compare("simb_pv") == 0) {
			dprint(Em: <comandos>: simb_pv lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			erro("\';\'");
		}
	}
}

void cmd(vector<string> local, vector<string> S){
	int ret, lin_completar, inicio_loop;	
	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	
	if(simb.compare("simb_read") == 0 || simb.compare("simb_write") == 0){
		dprint(Em: <cmd>: read-write lido pelo sintatico)
		int cmd_read = (simb.compare("simb_read") == 0) ? 1 : 0;		//se read cmd_read = 1, se write cmd_read = 0
		simb = obter_simbolo();
		
		// (
		if (simb.compare("simb_apar") == 0) {
			dprint(Em: <cmd>: simb_apar lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\'(\'", {"id"}, V);
			if (ret <= 0) return;
		}
		
		// <variaveis>
		int continua, ret;
		string tipo, tipo_cmd;
		int primeiro = 1;
		do {
			continua = 0;
			// ident
			if (simb.compare("id") == 0) {
				dprint(Em: <cmd>: id lido pelo sintatico)
				
				if (tabela.count(key(id)) == 1) {		//Busca na tabela
					//Verifica tipo
					tipo = tabela[key(id)].tipo;
					if(primeiro) { tipo_cmd = tipo;	primeiro = 0;}	//Define tipo do operação
					else if (tipo.compare(tipo_cmd) != 0) {
						erro_sem("Read/Write com tipos diferentes");
					}
				} else {
					erro_sem("Variavel nao declarada");
				}
							
				simb = obter_simbolo();
			} else {
				ret = erro("identificador", {"simb_v", "id"}, V);
				if (ret <= 0) break;
			}
			
			if(cmd_read) {
				gerar_cod("LEIT");
				gerar_cod("ARMZ " + to_string(tabela[key(id)].endereco));
			} else {
				gerar_cod("CRVL " + to_string(tabela[key(id)].endereco));
				gerar_cod("IMPR");
			}
			
			// ',' ou 'lambda'
			if (simb.compare("simb_v") == 0) {
				dprint(Em: <variaveis>: simb_v lido pelo sintatico)
				simb = obter_simbolo();
				continua = 1;
			}
		} while(continua);
		
		// )
		if (simb.compare("simb_fpar") == 0) {
			dprint(Em: <cmd>: simb_fpar lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\')\'", {"id"}, V);
			if (ret <= 0) return;
		}
	}
	else if (simb.compare("simb_while") == 0){
		dprint(Em: <cmd>: simb_while lido pelo sintatico)
		simb = obter_simbolo();
		
		// (
		if (simb.compare("simb_apar") == 0) {
			dprint(Em: <cmd>: simb_apar lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\'(\'", {"id"}, V);
			if (ret <= 0) return;
		}
		
		inicio_loop = lin_cod;		//Salva endereço do início do loop		
		// <condicao>
		condicao({"simb_fpar"}, V);
		
		// )
		if (simb.compare("simb_fpar") == 0) {
			dprint(Em: <cmd>: simb_fpar lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("\')\'", {"id"}, V);
			if (ret <= 0) return;
		}
		
		// do
		if (simb.compare("simb_do") == 0) {
			dprint(Em: <cmd>: simb_do lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("do", cmds, V);
			if (ret <= 0) return;
		}
		
		lin_completar = lin_cod;		//Guarda linha a ser completada
		gerar_cod("DSVF ");				//Necessário completar com endereço depois
		
		// <cmd>
		cmd(cmds, V);
		
		gerar_cod("DSVI " + to_string(inicio_loop));	
		if (!num_erros) cod[lin_completar] += to_string(lin_cod);	//Completa com end. de desvio while
	}
	else if (simb.compare("simb_if") == 0){
		dprint(Em: <cmd>: simb_if lido pelo sintatico)
		simb = obter_simbolo();
		
		// <condicao>
		condicao(cmds, V);
		
		// then
		if (simb.compare("simb_then") == 0) {
			dprint(Em: <cmd>: simb_then lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("then", cmds, V);
			if (ret <=0) return;
		}
		
		lin_completar = lin_cod;	//Guarda linha a ser completada
		gerar_cod("DSVF ");			//Necessário completar com endereço depois
		
		// <cmd>
		cmd(cmds, V);
		
		// else ou lambda
		if (simb.compare("simb_else") == 0) {
			dprint(Em: <cmd>: simb_else lido pelo sintatico)
			
			if (!num_erros) cod[lin_completar] += to_string(lin_cod + 1);	//Completa com end. de desvio if quando há else (+1 pula DSVI)
			
			//Código para pular else caso condição do if seja verdadeira
			lin_completar = lin_cod;		//Guarda linha a ser completada
			gerar_cod("DSVI ");				//Necessário completar com endereço depois
			
			simb = obter_simbolo();
			
			// <cmd>
			cmd(cmds, V);
			
			if (!num_erros) cod[lin_completar] += to_string(lin_cod);	//Completa com end. de desvio incondicional
			
		} else {
			if (!num_erros) cod[lin_completar] += to_string(lin_cod);	//Completa com end. de desvio if quando não há else
		}
				
				
	}
	else if (simb.compare("id") == 0){
		dprint(Em: <cmd>: id lido pelo sintatico)
		
		if (tabela.count(key(id)) == 0) {		//Busca na tabela
			erro_sem("Identificador nao declarado");
		}
		
		string name = id;		//Nome do procedimento ou da variável
		simb = obter_simbolo();
		
		// ":=", '(' or 'lambda'
		if (simb.compare("simb_atrib") == 0) {
			dprint(Em: <cmd>: simb_atrib lido pelo sintatico)
			simb = obter_simbolo();
			
			// <expressao>
			string tipo = expressao(cmds, V);
			if (tabela.count(key(name)) == 1 && tipo.compare("real") == 0 && tabela[key(name)].tipo.compare("inteiro") == 0) {				//Verificação de tipos (id continua salvo)
				erro_sem("Atribuicao de real a inteiro");
			}
			
			gerar_cod("ARMZ " + to_string(tabela[key(name)].endereco));
			
		} else if (simb.compare("simb_apar") == 0) {
			dprint(Em: <cmd>: simb_apar lido pelo sintatico)
			simb = obter_simbolo();
			
			
			lin_completar = lin_cod;	//Guarda linha a ser completada
			gerar_cod("PUSHER ");		//Necessário completar com endereço de retorno depois
			
			// <argumentos>
			argumentos({"simb_fpar", "simb_begin", "simb_read", "simb_write", "simb_for", "simb_if", "simb_while", "id"}, V, param[name]);
			
			gerar_cod("CHPR " + to_string(tabela[key(name)].endereco));
			if (!num_erros) cod[lin_completar] += to_string(lin_cod);	//Completa com end. de retorno
			
			// )
			if (simb.compare("simb_fpar") == 0) {
				dprint(Em: <cmd>: simb_fpar lido pelo sintatico)
				simb = obter_simbolo();
			} else {
				erro("\')\'", cmds, V);
			}
		}
	}
	else if (simb.compare("simb_begin") == 0){
		dprint(Em: <cmd>: simb_begin lido pelo sintatico)
		simb = obter_simbolo();
		
		// <comandos>
		comandos(V, {});
		
		// end
		if (simb.compare("simb_end") == 0) {
			dprint(Em: <cmd>: simb_end lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("end", cmds,V);
			if (ret <= 0);
		}
	}
	else if (simb.compare("simb_for") == 0){
		dprint(Em: <cmd>: simb_for lido pelo sintatico)
		simb = obter_simbolo();
		
		string id_for_end; //Endereço do ident do for
		
		// ident
		if (simb.compare("id") == 0) {
			dprint(Em: <cmd>: id lido pelo sintatico)
			
			if (tabela.count(key(id)) == 0) {		//Busca na tabela
				erro_sem("Identificador nao declarado");
			} else id_for_end = to_string(tabela[key(id)].endereco);
			
			simb = obter_simbolo();
		} else {
			ret = erro("identificador", {"simb_atrib"}, V);
			if (ret <= 0) return;
		}
			
		// :=
		if (simb.compare("simb_atrib") == 0) {
			dprint(Em: <cmd>: simb_atrib lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro(":=", {"simb_mais", "simb_menos", "simb_apar", "id", "NUM_INT", "NUM_REAL"}, V);
			if (ret <= 0) return;
		}
		
		// <expressao>
		expressao({"simb_to"}, V);
		
		gerar_cod("ARMZ " + id_for_end);	//<ident> := <expressao>
		
		// to
		if (simb.compare("simb_to") == 0) {
			dprint(Em: <cmd>: simb_to lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("to", {"simb_mais", "simb_menos", "simb_apar", "id", "NUM_INT", "NUM_REAL"}, V);
			if (ret <= 0) return;
		}
		
		inicio_loop = lin_cod;			
		// <expressao>
		expressao({"simb_do", "simb_begin", "simb_read", "simb_write", "simb_for", "simb_if", "simb_while", "id"}, V);
		
		// do
		if (simb.compare("simb_do") == 0) {
			dprint(Em: <cmd>: simb_do lido pelo sintatico)
			simb = obter_simbolo();
		} else {
			ret = erro("do", cmds, V);
			if (ret <= 0) return;
		}
		
		gerar_cod("CRVL " + id_for_end);
		gerar_cod("CMAI");
		lin_completar = lin_cod;		//Guarda linha a ser completada
		gerar_cod("DSVF ");				//Necessário completar com endereço depois
		
		// <cmd>
		cmd(cmds, V);
		
		gerar_cod("CRVL " + id_for_end);
		gerar_cod("CRCT 1");
		gerar_cod("SOMA");
		gerar_cod("ARMZ " + id_for_end);				//<ident> := <ident> + 1;
		gerar_cod("DSVI " + to_string(inicio_loop));	//Volta ao for
		
		if (!num_erros) cod[lin_completar] += to_string(lin_cod);	//Completa com end. de desvio for
	
	}
	else {
		erro("Comando", {"simb_pv"});
	}
}

void condicao(vector<string> local, vector<string> S){
	int ret;	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	string rel;
	
	expressao({"simb_igual", "simb_dif", "simb_maior_igual", "simb_menor_igual", "simb_maior", "simb_menor"}, V);
	
	if(	simb.compare("simb_igual")		== 0 ||
		simb.compare("simb_dif")		== 0 ||
		simb.compare("simb_maior_igual")== 0 ||
		simb.compare("simb_menor_igual")== 0 ||
		simb.compare("simb_maior")		== 0 ||
		simb.compare("simb_menor")		== 0 )
	{
		dprint(Em: <condicao>: simbolo de relacao condicional lido pelo sintatico)
		rel = simb;				//Guarda relação condicional lida
		simb = obter_simbolo();
	} else {
		ret = erro("Simbolo de comparacao", {"simb_mais", "simb_menos", "simb_apar", "id", "NUM_INT", "NUM_REAL"}, V);
		if (ret <= 0) return;
	}	
	
	expressao({}, V);
	if (!num_erros) {
		if(rel.compare("simb_igual") == 0 ) gerar_cod("CPIG");
		else if(rel.compare("simb_dif") == 0 ) gerar_cod("CDES");
		else if(rel.compare("simb_maior_igual") == 0 ) gerar_cod("CMAI");
		else if(rel.compare("simb_menor_igual") == 0 ) gerar_cod("CPMI");
		else if(rel.compare("simb_maior") == 0 ) gerar_cod("CPMA");
		else if(rel.compare("simb_menor") == 0 ) gerar_cod("CPME");
	}
}

string expressao(vector<string> local, vector<string> S){
	int ret, soma;
	string tipo, tipo2;	
	vector<string> V(local);
	move(S.begin(), S.end(), back_inserter(V));
	
	tipo = termo(V);
	
	soma = 0;	// Indica se haverá soma ou subtração
	if(	simb.compare("simb_mais") == 0 || simb.compare("simb_menos") == 0 )
	{
		dprint(Em: <expressao>: simb_mais ou simb_menos lido pelo sintatico)
		if (simb.compare("simb_mais") == 0) soma = 1;
		
		simb = obter_simbolo();
		
		tipo2 = expressao({}, V);
		if (tipo2.compare("real") == 0) tipo = "real";	//Possível coerção de inteiro para real
		
		if (soma) gerar_cod("SOMA");
		else gerar_cod("SUBT");
	}
	return tipo;
}

string termo(vector<string> S){
	int ret, div, mult, inve;
	string tipo, id_salvo;
	tipo = "";
	inve = 0;	//Indica se precisa inveter sinal
	
	// '+', '-', ou 'lambda'
	if(	simb.compare("simb_mais") == 0 ||simb.compare("simb_menos")	== 0 )
	{
		dprint(Em: <termo>: simb_mais ou simb_menos lido pelo sintatico)
		inve = 1;
		simb = obter_simbolo();
	}
	
	mult = 0;	//Indica se está na operação de multiplicação
	div = 0;	//Indica se está na operação de divisão
	while(1){
		// <numero>, '(', ou ident
		if (simb.compare("simb_apar") == 0) {
			dprint(Em: <termo>: simb_apar lido pelo sintatico)
			simb = obter_simbolo();
			
			tipo = expressao({"simb_fpar"}, S);
			
			if (simb.compare("simb_fpar") == 0) {
				dprint(Em: <termo>: simb_fpar lido pelo sintatico)
				simb = obter_simbolo();
			} else {
				ret = erro("\')\'", {"simb_div", "simb_mult"}, S);
				if (ret <= 0) return tipo;
			}
			
		} else if (	simb.compare("NUM_REAL") == 0) {
			dprint(Em: <termo>: <numero> lido pelo sintatico)
			tipo = "real";
			gerar_cod("CRCT " + val);
			simb = obter_simbolo();
			
		} else if (simb.compare("NUM_INT")  == 0) {
			dprint(Em: <termo>: <numero> lido pelo sintatico)
			if (tipo.compare("real") != 0) tipo = "inteiro";	//Possível coerção
			gerar_cod("CRCT " + val);
			simb = obter_simbolo();	
					
		} else if (simb.compare("id") == 0) {
			dprint(Em: <termo>: <id> lido pelo sintatico)
			if (tabela.count(key(id)) == 0) erro_sem("Identificador nao declarado");		//Busca na tabela se é identificador
			else {
				elemento e = tabela[key(id)];
				if (tipo.compare("real") != 0) tipo = e.tipo;		//Atribui tipo com Possível coerção
				if (e.categoria.compare("const") == 0 && tipo.compare("real") == 0) gerar_cod("CRCT " + to_string(e.v_real));	//Constante
				else if (e.categoria.compare("const") == 0) gerar_cod("CRCT " + to_string(e.v_inteiro));
				else gerar_cod("CRVL " + to_string(e.endereco));		//Variável
			}
			
			simb = obter_simbolo();
		
		} else {
			ret = erro("Numero ou identificador", {"simb_div", "simb_mult"}, S);
			if (ret <= 0) return "";
		}
		
		if (div) {
			if (tipo.compare("inteiro") != 0) erro_sem("Divisao entre nao inteiros");
			gerar_cod("DIVI");
		} else if (mult) gerar_cod("MULT");
		
		// '*', '/', ou 'lambda'
		if (simb.compare("simb_mult") == 0) {
			dprint(Em: <termo>: simb_mult lido pelo sintatico)
			mult = 1;
			div = 0;
			simb = obter_simbolo();
			continue;
		} else if (simb.compare("simb_div") == 0) {
			dprint(Em: <termo>: simb_div lido pelo sintatico)
			if (tipo.compare("inteiro") != 0) erro_sem("Divisao entre nao inteiros");
			div = 1;
			mult = 0;
			simb = obter_simbolo();
			continue;
		} else {
			break;
		}
	}
	
	if (inve) gerar_cod("INVE");
	
	return tipo;
}


void erro() {
	num_erros++;
	cout << "Erro na linha " << linha << " : " << simb << endl;
}

void erro(string esp) {
	num_erros++;
		if (simb.compare("id") == 0) {
		cout << "Erro na linha " << linha << " : " << id << ". Esperado : " << esp << endl;
	} else {
		cout << "Erro na linha " << linha << " : " << simb << ". Esperado : " << esp << endl;
	}
}

/*Retorno == -1 -> Seguidor do pai encontrado
  Retorno == 0 ->  Simb. Sincr. não encontrado
  Retorno == 1 ->  Simb. Sincr. encontrado (não do pai)
*/
int erro(string esp, vector<string> seg, vector<string> segPai) {
	num_erros++;
	if (simb.compare("id") == 0) {
		cout << "Erro na linha " << linha << " : " << id << ". Esperado : " << esp << endl;
	} else {
		cout << "Erro na linha " << linha << " : " << simb << ". Esperado : " << esp << endl;
	}
	//Procura símbolos de sincronização próprios
	while (!ler.eof()) {
		if ( find(seg.begin(), seg.end(), simb) != seg.end() ) return 1;
		if ( find(segPai.begin(), segPai.end(), simb) != segPai.end() ) return -1;
		simb = obter_simbolo();
	}
	return 0;
}

//Não recebe seguidores do pai
int erro(string esp, vector<string> seg) {
	num_erros++;
	if (simb.compare("id") == 0) {
		cout << "Erro na linha " << linha << " : " << id << ". Esperado : " << esp << endl;
	} else {
		cout << "Erro na linha " << linha << " : " << simb << ". Esperado : " << esp << endl;
	}
	//Procura símbolos de sincronização próprios
	while (!ler.eof()) {
		if ( find(seg.begin(), seg.end(), simb) != seg.end() ) return 1;
		simb = obter_simbolo();
	}
	return 0;
}

//Erro semântico
void erro_sem(string msg) {
	num_erros++;
	cout << "Erro na linha " << linha << " : " << id << ". " << msg << endl;
}
