//
// Created by estuardo on 13/02/23.
//

#include "NodoMount.h"

NodoMount::NodoMount() {
    this->nombre_disco = "";
    this->numero = 0;
    this->part_start = -1;
    this->part_type = '\0';
    this->fichero = "";
    this->id = "";
    this->idCompleto = "";
    this->next = NULL;
    this->nombre_particion = "";
}