//
// Created by estuardo on 16/02/23.
//

#ifndef P1_MIA_202003894_USUARIO_H
#define P1_MIA_202003894_USUARIO_H

#include <string>
using namespace std;

class Usuario {
public:
    int idU;
    int idG;
    string nombreG;
    string nombreU;
    string password;
    string idParticion;

    Usuario();
    void borrarInfoU();
    void ingresarInfoU(int idG, int idU, string nombreG, string nombreU, string password, string idParticion);
};

#endif //P1_MIA_202003894_USUARIO_H
