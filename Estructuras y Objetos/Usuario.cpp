//
// Created by estuardo on 16/02/23.
//

#include "Usuario.h"

Usuario::Usuario() {
    this->idG = 0;
    this->idU = 0;
    this->nombreG = "";
    this->nombreU = "";
    this->password = "";
    this->idParticion = "";
}

void Usuario::ingresarInfoU(int idG, int idU, string nombreG, string nombreU, string password, string idParticion) {
    this->idG = idG;
    this->idU = idU;
    this->nombreG = nombreG;
    this->nombreU = nombreU;
    this->password = password;
    this->idParticion = idParticion;
}

void Usuario::borrarInfoU() {
    this->idG = 0;
    this->idU = 0;
    this->nombreG = "";
    this->nombreU = "";
    this->password = "";
    this->idParticion = "";
}