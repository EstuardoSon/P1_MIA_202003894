//
// Created by estuardo on 11/02/23.
//

#ifndef P1_MIA_202003894_ANALIZADOR_H
#define P1_MIA_202003894_ANALIZADOR_H

#include <string>
#include "../Estructuras y Objetos/Estructura.h"
#include "../Estructuras y Objetos/ListaMount.h"

using namespace std;

class Analizador {
public:
    Analizador(string cadena, ListaMount *listaMount);
    ListaMount * listaMount;
    string cadena;

    void analizar();
    void obtenerDatosPath(string &fichero, string &nombre, int tamanio);
    void obtenerDatoParamS(string &parametro, int tamanio);
    void obtenerDatoParamN(int &parametro, int tamanio);
    void obtenerDatoParamC(string &parametro, int tamanio);
    int recoInstruccion(string cadena);
    int obtenerPosEspacio(string cadena);
    int obtenerPosEnd(string cadena);
    bool verificarComentario(string cadena);
    bool verificarExtension(string cadena);
    string trim(string cadena);
    string toLower(string cadena);
    string obtenerNombre(string &cadena);
};


#endif //P1_MIA_202003894_ANALIZADOR_H
