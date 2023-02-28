//
// Created by estuardo on 13/02/23.
//

#ifndef P1_MIA_202003894_REPORTE_H
#define P1_MIA_202003894_REPORTE_H

#include <string>
#include <vector>
#include "NodoMount.h"
#include "ListaMount.h"
#include "Estructura.h"

using namespace std;

class Reporte {
public:
    ListaMount * listaMount;
    string name;
    string fichero_p;
    string archivo_p;
    string fichero_r;
    string archivo_r;
    string id;
    string ruta;

    Reporte(ListaMount * listaMount);
    NodoMount * verificarID();
    string obtenerExtension(string nombreArchivo);
    bool verificarRuta(string fichero, string archivo, string extension);
    void generarReporte();
    void reporteMBR();
    void crearParticionRMBR(FILE * archivoReporte, FILE * archivoDisco, Partition & partition);
    void crearParticionLRMBR(FILE * archivoReporte, FILE * archivoDisco, int part_start);
    void reporteDisk();
    void reporteSb();
    void reporteBmInode();
    void reporteBmBlock();
};


#endif //P1_MIA_202003894_REPORTE_H
