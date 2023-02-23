//
// Created by estuardo on 13/02/23.
//

#ifndef P1_MIA_202003894_PARTICION_H
#define P1_MIA_202003894_PARTICION_H

#include <string>
#include "Estructura.h"
#include "ListaMount.h"

using namespace std;

class Particion {
public:
    int s;
    int add;
    string u;
    string path;
    string nombre_disco;
    string t;
    string f;
    string delete_;
    string name;

    Particion();
    void fdisk();
    void mostrarDatosP(Partition &p);
    void mostrarDatosEBR(EBR &ebr);
    Partition crearStructPartition(char tipo);
    Partition buscarParticionE(MBR &mbr);
    bool verificarNombrePL(Partition &partition, char &tipo);
    bool verificarNombreP(MBR &mbr, char &tipo);
    bool verificarEspacio(string path, int tipoP);
    bool verificarEspacioPE(Partition &p, EBR &nextEBR);
    bool verificarPE(MBR &mbr);
    int obtenerInicioP();
    int verificarTamanio();
    int verificarTipoP();
    int verificarAjusteP();
    int obtenerInincioSiguiente(MBR &mbr, int porParticion);
    int obtenerSizeDP(MBR &mbr);
};


#endif //P1_MIA_202003894_PARTICION_H
