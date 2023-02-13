#include <iostream>
#include <regex>
#include "Analizador/Analizador.h"

using namespace std;

string toLower(string cadena);

int main() {
    cout << "Proyecto 1 - MIA " << endl;
    cout << "Estuardo Gabriel Son Mux - 202003894 " << endl;
    cout << "Escriba Exit para finalizar la ejecucion " << endl;
    cout << " ------------------------------------------ " << endl;
    string comando = "";
    string exit = "exit";

    while (true){
        fflush(stdin);
        char comandoC [1024];
        cout << ">>";
        cin.getline(comandoC, sizeof(comandoC), '\n');
        comando = comandoC;

        if ((strcmp(toLower(comando).c_str(), exit.c_str())) == 0){
            break;
        }

        Analizador *analizador = new Analizador(comando);
        analizador->analizar();
    }
    return 0;
}

//Convertir una cadena a minusculas
string toLower(string cadena){
    for (int x=0; x<cadena.length(); x++) {
        cadena[x] = tolower(cadena[x]);
    }

    return cadena;
}
