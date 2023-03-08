//
// Created by estuardo on 26/02/23.
//

#ifndef P1_MIA_202003894_GESTORARCHIVOS_H
#define P1_MIA_202003894_GESTORARCHIVOS_H


#include "ListaMount.h"
#include "Usuario.h"
#include <string.h>
#include <iostream>
#include <vector>

using namespace std;

class GestorArchivos {
public:
    ListaMount * listaMount;
    Usuario * usuario;

    GestorArchivos(ListaMount * listaMount, Usuario * usuario);
    void chmod(string path, int ugo, bool r);
    void mkfile(string path_fichero, string path_archivo, bool r, int size, string cont_fichero, string cont_archivo);
    void cat(vector<string> rutas);
    void remove(string path);
    void edit(string path, string contenido);
    void rename(string path, string nombre);
    void mkdir(string path, bool r);
    void move(string path, string destino);
    void find(string path, string name);
    void chown(string path, string usr, bool r);

    int buscarficheroChmod(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    void buscarfichero(vector<string> &ficheros, string nombreArchivo, bool r, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo,string textoArchivo);
    void buscarficheroCat(vector<string> &ficheros, string nombreArchivo, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    int buscarficheroRemove(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    void buscarficheroMkdir(vector<string> &ficheros, string newCarpeta, bool r, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    bool buscarficheroMove(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo, int inicioMove, string nombreMove);

    int buscarBM_b(SuperBloque &sb, FILE * archivo);
    int buscarBM_i(SuperBloque &sb, FILE * archivo);

    void verificarPermisos(TablaInodo & inodo, bool &escritura, bool &lectura);

    void crearCarpeta(FILE *archivo, SuperBloque &sb, int &ubicacion, int inicioInodo);
    void crearArchivo(FILE *archivo, string textoArchivo, SuperBloque &sb, int inicioSB);

    void buscarEnCarpetaChmod(int ubicacion, int ugo, FILE *archivo);
    int buscarEnCarpeta(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre);
    bool buscarEspacio(TablaInodo &ti, int incioInodo, SuperBloque &sb, int inicioSB, FILE * archivo, int inicioMove, string nombreMove);
    void buscarEnCarpetaFind(int inicioInodo, SuperBloque &sb, int inicioSB, FILE *archivo, string nombre, int identacion);
    void buscarEnCarpetaChown(int inicioInodo, SuperBloque &sb, int inicioSB, FILE *archivo, int u, int g, bool r);

    int buscarEspacioArchivo(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre, SuperBloque &sb, int inicioSB, string textoArchivo);
    int buscarEspacioCarpeta(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre, SuperBloque &sb, int inicioSB);

    bool eliminar(int inicioInodo, FILE * archivo, SuperBloque &sb, int inicioSB);

    bool verificarPath(string cadena);
    bool verificarUGO(int ugo);
    bool verificarArchivo(string cadena);

    void writeInFile(string texto, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo);
    vector<string> split(string cadena,char delim);
    string getContentF(int inicioInodo, FILE *archivo);
    void obtenerUG(string cadena, vector<string> & usuarios, vector<string> & grupos);
};


#endif //P1_MIA_202003894_GESTORARCHIVOS_H
