//
// Created by estuardo on 16/02/23.
//

#ifndef P1_MIA_202003894_ADMINUSUARIO_H
#define P1_MIA_202003894_ADMINUSUARIO_H

#include "Usuario.h"
#include "ListaMount.h"
#include "NodoMount.h"
#include <string>
#include <vector>

using namespace std;

class AdminUsuario {
public:
    ListaMount * listaMount;
    NodoMount * nodoMount;
    Usuario * usuario;
    string idParticion;

    AdminUsuario(ListaMount * listaMount, Usuario * usuario);
    void login(string usuario, string password, string id);
    void logout();
    void mkgrp(string name);
    void rmgrp(string name);
    void writeInFile(string texto, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    int buscarBM_b(SuperBloque &sb, FILE * archivo);
    void obtenerUG(string cadena, vector<string> & usuarios, vector<string> & grupos);
    string getContentF(int inicioInodo, FILE * archivo);
    vector<string> split(string cadena, char delim);
    string trim(std::string cadena);
};


#endif //P1_MIA_202003894_ADMINUSUARIO_H
