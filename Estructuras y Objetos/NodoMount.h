//
// Created by estuardo on 13/02/23.
//

#ifndef P1_MIA_202003894_NODOMOUNT_H
#define P1_MIA_202003894_NODOMOUNT_H


#include <string>
using namespace std;

class NodoMount {
public:
    string idCompleto;
    string nombre_particion;
    string id;
    int numero;
    int part_start;
    char part_type;
    string fichero;
    string nombre_disco;
    NodoMount * next;

    NodoMount();
};


#endif //P1_MIA_202003894_NODOMOUNT_H
