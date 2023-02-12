//
// Created by estuardo on 11/02/23.
//

#include "Disco.h"
#include "Estructura.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <regex>

using namespace std;

Disco::Disco() {
    this->s = -1;
    this->f = "";
    this->u = "";
    this->path = "";
    this->nombre ="";
}

int Disco::verificarAjusteD() {
    if (this->f == "bf"){ return 1; }
    else if (this->f == "ff" || this->f == ""){ return 2; }
    else if (this->f == "wf"){ return 3; }
    return -1;
}

int Disco::verificarTamanio() {
    if (this->u == "m" || this->u == ""){ return 1024; }
    else if (this->u == "k"){ return 1; }
    return -1;
}

void Disco::mkdisk() {
    //Verificacion de datos para la cracion del disco
    int tamanio = this->verificarTamanio();
    int ajusteD = this->verificarAjusteD();
    if (this->s <= 0 || ajusteD == -1 || tamanio == -1){
        cout << "No fue posible crear el disco con la informacion proporcionada" << endl;
        cout << endl;
        return;
    }

    regex expresion ("(\\/[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+\\/)([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.dsk)");
    if (!regex_match(this->path+"/"+this->nombre,expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return;
    }

    //Verificacion de la existencia del archivo
    string direccion = this->path+"/"+this->nombre;

    FILE * file = fopen(direccion.c_str(),"rb");

    if(file != NULL){
        cout << "El archivo ya existe" << endl;
        cout << endl;
        fclose(file);
        return;
    }

    //Creacion de los ficheros y dando permisos
    string comando = "sudo -S mkdir -p \'" + this->path + "\'";
    system(comando.c_str());
    comando = "sudo -S chmod -R 777  \'" + this->path + "\'";
    system(comando.c_str());

    //Creacion del Archivo y llenandolo de caracteres vacios
    file = fopen(direccion.c_str(),"wb");

    char bytes[1024];
    for (int i = 0; i < 1024; i++) {
        bytes[i] = '\0';
    }

    this->s = this->s * tamanio;
    for (int i = 0 ; i <  this->s; i++) {
        fwrite(bytes, sizeof(bytes), 1,file);
    }

    //Regresar el buffer de lectura del archivo al inicio
    fseek(file,0,SEEK_SET);

    //Crear el MBR para
    MBR mbr;
    mbr.mbr_tamano  = 1024 * this->s;
    mbr.mbr_fecha_creacion = time(nullptr);
    mbr.mbr_dsk_signature = static_cast<int>(time(nullptr));
    if (ajusteD == 1) mbr.dsk_fit = 'B';
    else if (ajusteD == 2) mbr.dsk_fit = 'F';
    else if (ajusteD == 3) mbr.dsk_fit = 'W';
    for (int i = 0; i<4; i++){
        mbr.mbr_partition_[i].part_status = '0';
        mbr.mbr_partition_[i].part_start = -1;
        mbr.mbr_partition_[i].part_s = 0;
    }

    fwrite(&mbr, sizeof(mbr), 1,file);

    fclose(file);

    //cout << this->obtenerNombreRaid() << endl;
    cout << "- Disco creado con exito -" << endl;
    cout << "Nombre: " << this->nombre << endl;
    cout << "Size: " << mbr.mbr_tamano << endl;
    cout << "Fecha C: " << mbr.mbr_fecha_creacion << endl;
    cout << "Num ID: " << mbr.mbr_dsk_signature << endl;
    cout << "Ajuste: " << mbr.dsk_fit << endl << endl;
}

void Disco::rmdisk() {
    regex expresion ("(\\/[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+\\/)([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.dsk)");
    if (!regex_match(this->path+"/"+this->nombre,expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return;
    }

    //Verificando la existencia del archivo
    string fullpath = this->path+"/"+this->nombre;
    FILE *file = fopen(fullpath.c_str(),"rb");

    if (file != NULL){
        fclose(file);

        cout << "El archivo con la direccion especificado fue encontrado, seguro que quiere eliminarlo [Y/n]" << endl;
        cout << ">>";
        string opcion;
        getline(cin, opcion);

        if(opcion == "Y" || opcion == "y"){
            remove(fullpath.c_str());
            cout << "Archivo eliminado con Exito" << endl << endl;
        }
        else if (opcion == "n" || opcion == "N") cout << "Cancelando Eliminacion" << endl << endl;
        else cout << "Opcion no valida, no pudo ejecutarse la accion" << endl << endl;
    }
    else cout << "El archivo especificado no existe" << endl << endl;
}
