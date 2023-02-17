//
// Created by estuardo on 13/02/23.
//

#ifndef P1_MIA_202003894_LISTAMOUNT_H
#define P1_MIA_202003894_LISTAMOUNT_H


#include "NodoMount.h"
#include "Estructura.h"
#include <string>

using namespace std;

class ListaMount {
public:
    NodoMount * inicio;
    NodoMount * fin;

    ListaMount();

    int obtenerType(string type);
    int obtenerFs(string fs);
    void agregar(NodoMount * nuevo);
    void obtenerIdNumero(NodoMount * nuevo);
    void imprimirLista();
    void mkfs(string id, string type, string fs);
    void ext(int type, int sistemaArchivo, NodoMount * nodo);
    bool verificarRuta(NodoMount * nuevo);
    bool verificarParticion(FILE * archivo, MBR & mbr, NodoMount * nuevo);
    bool verificarParticionL(FILE * archivo, Partition & p, NodoMount * nuevo);
    bool verificarID(NodoMount * nuevo);
    bool verificarLista(NodoMount * nuevo);
    NodoMount * eliminar(string idCompleto);
    NodoMount * buscar(string idCompleto);
};


#endif //P1_MIA_202003894_LISTAMOUNT_H
