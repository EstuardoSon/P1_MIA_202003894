//
// Created by estuardo on 11/02/23.
//

#ifndef P1_MIA_202003894_DISCO_H
#define P1_MIA_202003894_DISCO_H

#include <string>

using namespace std;

class Disco {
    Disco();
    int s;
    string f;
    string u;
    string path;
    string nombre;

    void mkdisk();
    void rmdisk();
    int verificarAjusteD();
    int verificarTamanio();
};


#endif //P1_MIA_202003894_DISCO_H
