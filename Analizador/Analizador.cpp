//
// Created by estuardo on 11/02/23.
//

#include "Analizador.h"
#include "../Estructuras y Objetos/Disco.h"
#include "../Estructuras y Objetos/Particion.h"
#include <regex>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <string.h>

using namespace std;

Analizador::Analizador(std::string cadena) {
    this->cadena = this->trim(cadena);
}

//Convertir en Minusculas
string Analizador::toLower(string cadena){
    for (int x=0; x<cadena.length(); x++) {
        cadena[x] = tolower(cadena[x]);
    }
    return cadena;
}

//Eliminar espacios en blacon al inicio y final
string Analizador::trim(std::string cadena) {
    cadena.erase(cadena.begin(), std::find_if( cadena.begin(), cadena.end(),std::not1(std::ptr_fun<int, int>(std::isspace))));
    cadena.erase(std::find_if(cadena.rbegin(), cadena.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), cadena.end());
    return cadena;
}

//Obtener el nombre del disco de la direccion
string Analizador::obtenerNombre(std::string &cadena) {
    int pos = cadena.find('/');
    int aux = cadena.find('/',pos+1);

    if(pos != -1){
        while(aux != -1){
            pos = aux;
            aux = cadena.find('/',aux+1);
        }
        string nombre = cadena.substr(pos+1);
        cadena.erase(pos);
        return nombre;
    }
    return "";
}

//Verificar la extension del archivo para el comando exec
bool Analizador::verificarExtension(std::string cadena) {
    int posExtension = cadena.find(".eea");

    if(posExtension == -1 || posExtension != (cadena.length()-4)){
        cout << "La extension del archivo no es la correcta" << endl;
        cout << endl;
        return false;
    }
    return true;
}

//Verificar si la cadena es un comentario
bool Analizador::verificarComentario(std::string cadena) {
    string comentario = "#";

    if(strncmp(this->trim(cadena).c_str(), comentario.c_str(), 1) == 0){
        cout << cadena << endl ;
        return true;
    }
    return false;
}

//Obtener la posicion de la ultima "
int Analizador::obtenerPosEnd(std::string cadena) {
    for (int x = 0 ; x < cadena.length() ; x++){
        if (cadena[x] == '"'){
            return x;
        }
    }
    return cadena.length();
}

//Identificar el tipo de instruccion
int Analizador::recoInstruccion(std::string cadena) {
    string mkdisk ="mkdisk";
    string rmdisk = "rmdisk";
    string fdisk = "fdisk";
    string mount = "mount";
    string unmount = "unmount";
    string mkfs = "mkfs";
    string login = "login";
    string logout = "logout";
    string mkgrp = "mkgrp";
    string rmgrp = "rmgrp";
    string mkusr = "mkusr";
    string rkusr = "rmusr";
    string chmod = "chmod";
    string mkfile = "mkfile";
    string cat = "cat";
    string remove = "remove";
    string edit = "edit";
    string rename = "rename";
    string mkdir = "mkdir";
    string copy = "copy";
    string move = "move";
    string find = "find";
    string chown = "chown";
    string chgrp = "chgrp";
    string pause = "pause";
    string recovery = "recovery";
    string loss = "loss";
    string exec = "exec";
    string rep = "rep";

    if (strncmp(this->toLower(cadena).c_str(),mkdisk.c_str(),mkdisk.length()) == 0){ return 1; }
    else if (strncmp(this->toLower(cadena).c_str(),rmdisk.c_str(),rmdisk.length()) == 0){ return 2; }
    else if (strncmp(this->toLower(cadena).c_str(),fdisk.c_str(),fdisk.length()) == 0){ return 3; }
    else if (strncmp(this->toLower(cadena).c_str(),mount.c_str(),mount.length()) == 0){ return 4; }
    else if (strncmp(this->toLower(cadena).c_str(),unmount.c_str(),unmount.length()) == 0){ return 5; }
    else if (strncmp(this->toLower(cadena).c_str(),mkfs.c_str(),mkfs.length()) == 0){ return 6; }
    else if (strncmp(this->toLower(cadena).c_str(),login.c_str(),login.length()) == 0){ return 7; }
    else if (strncmp(this->toLower(cadena).c_str(),logout.c_str(),logout.length()) == 0){ return 8; }
    else if (strncmp(this->toLower(cadena).c_str(),mkgrp.c_str(),mkgrp.length()) == 0){ return 9; }
    else if (strncmp(this->toLower(cadena).c_str(),rmgrp.c_str(),rmgrp.length()) == 0){ return 10; }
    else if (strncmp(this->toLower(cadena).c_str(),mkusr.c_str(),mkusr.length()) == 0){ return 11; }
    else if (strncmp(this->toLower(cadena).c_str(),rkusr.c_str(),rkusr.length()) == 0){ return 12; }
    else if (strncmp(this->toLower(cadena).c_str(),chmod.c_str(),chmod.length()) == 0){ return 13; }
    else if (strncmp(this->toLower(cadena).c_str(),mkfile.c_str(),mkfile.length()) == 0){ return 14; }
    else if (strncmp(this->toLower(cadena).c_str(),cat.c_str(),cat.length()) == 0){ return 15; }
    else if (strncmp(this->toLower(cadena).c_str(),remove.c_str(),remove.length()) == 0){ return 16; }
    else if (strncmp(this->toLower(cadena).c_str(),edit.c_str(),edit.length()) == 0){ return 17; }
    else if (strncmp(this->toLower(cadena).c_str(),rename.c_str(),rename.length()) == 0){ return 18; }
    else if (strncmp(this->toLower(cadena).c_str(),mkdir.c_str(),mkdir.length()) == 0){ return 19; }
    else if (strncmp(this->toLower(cadena).c_str(),copy.c_str(),copy.length()) == 0){ return 20; }
    else if (strncmp(this->toLower(cadena).c_str(),move.c_str(),move.length()) == 0){ return 21; }
    else if (strncmp(this->toLower(cadena).c_str(),find.c_str(),find.length()) == 0){ return 22; }
    else if (strncmp(this->toLower(cadena).c_str(),chown.c_str(),chown.length()) == 0){ return 23; }
    else if (strncmp(this->toLower(cadena).c_str(),chgrp.c_str(),chgrp.length()) == 0){ return 24; }
    else if (strncmp(this->toLower(cadena).c_str(),pause.c_str(),pause.length()) == 0){ return 25; }
    else if (strncmp(this->toLower(cadena).c_str(),recovery.c_str(),recovery.length()) == 0){ return 26; }
    else if (strncmp(this->toLower(cadena).c_str(),loss.c_str(),loss.length()) == 0){ return 27; }
    else if (strncmp(this->toLower(cadena).c_str(),exec.c_str(),exec.length()) == 0){ return 28; }
    else if (strncmp(this->toLower(cadena).c_str(),rep.c_str(),rep.length()) == 0){ return 29; }
    return -1;
}

//Obtener la posicion del siguiente espacio en blanco
int Analizador::obtenerPosEspacio(string cadena) {
    for (int x = 0; x < cadena.length(); x++){
        if (isspace(cadena[x])){
            return x;
        }
    }
    return cadena.length();
}

//Obtener el nombre del archivo y separarlo de los ficheros
void Analizador::obtenerDatosPath(std::string &fichero, std::string &nombre, int tamanio) {
    this->obtenerDatoParamC(fichero, tamanio);
    nombre = this->obtenerNombre(fichero);
}

//Obtener un parametro que puede estar entre comillas o sin comillas
void Analizador::obtenerDatoParamC(string &parametro, int tamanio) {
    this->cadena = this->trim(this->cadena.erase(0,tamanio));

    if (this->cadena[0] == '"') {
        this->cadena = this->cadena.erase(0, 1);
        int posComilla = this->obtenerPosEnd(this->cadena);
        parametro = this->cadena.substr(0, posComilla);
        this->cadena = this->trim(this->cadena.erase(0, posComilla + 1));
    } else {
        int posEspacio = this->obtenerPosEspacio(this->cadena);
        parametro = this->cadena.substr(0, posEspacio);
        this->cadena = this->trim(this->cadena.erase(0, posEspacio));
    }
}

//Obtener un parametro que no puede estar entre comillas
void Analizador::obtenerDatoParamS(std::string &parametro, int tamanio) {
    this->cadena = this->trim(this->cadena.erase(0,tamanio));
    int posEspacio = this->obtenerPosEspacio(this->cadena);
    parametro = this->toLower(this->cadena.substr(0,posEspacio));
    this->cadena = this->trim(this->cadena.erase(0,posEspacio));
}

//Obtener un parametro numerico
void Analizador::obtenerDatoParamN(int &parametro, int tamanio) {
    this->cadena = this->trim(this->cadena.erase(0,tamanio));
    int posEspacio = this->obtenerPosEspacio(this->cadena);
    parametro = stoi(this->cadena.substr(0,posEspacio));
    this->cadena = this->trim(this->cadena.erase(0,posEspacio));
}

void Analizador::analizar() {
    if (this->verificarComentario(this->cadena)){
        return;
    }

    cout << this->cadena << endl;
    int tipoInst = this->recoInstruccion(this->cadena);

    //Comando Mkdisk
    if (tipoInst == 1) {
        this->cadena = this->trim(this->cadena.erase(0, 6));
        string path_param = ">path=";

        Disco *disco = new Disco();

        while(this->cadena.length()>0){
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(disco->path,disco->nombre, path_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        disco->mkdisk();
    }

        //Comando Rmdisk
    else if (tipoInst == 2) {
        this->cadena = this->trim(this->cadena.erase(0, 6));

        string path_param = ">path=";
        Disco *disco = new Disco();

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(disco->path,disco->nombre,path_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        disco->rmdisk();
    }

        //Comando Fdisk
    else if(tipoInst == 3){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string s_param = ">size=";
        string u_param = ">unit=";
        string path_param = ">path=";
        string t_param = ">type=";
        string f_param = ">fit=";
        string delete_param = ">delete=";
        string name_param = ">name=";
        string add_param = ">add=";

        Particion *particion = new Particion();

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro S
            else if (strncmp(this->toLower(this->cadena).c_str(),s_param.c_str(),s_param.length())==0){
                this->obtenerDatoParamN(particion->s, s_param.length());
            }

                //Reconocer el parmatro U
            else if (strncmp(this->toLower(this->cadena).c_str(),u_param.c_str(),u_param.length())==0){
                this->obtenerDatoParamS(particion->u, u_param.length());
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(particion->path, particion->nombre_disco,path_param.length());
            }

                //Reconocer el parmatro T
            else if (strncmp(this->toLower(this->cadena).c_str(),t_param.c_str(),t_param.length())==0){
                this->obtenerDatoParamS(particion->t, t_param.length());
            }

                //Reconocer el parmatro F
            else if (strncmp(this->toLower(this->cadena).c_str(),f_param.c_str(),f_param.length())==0){
                this->obtenerDatoParamS(particion->f, f_param.length());
            }

                //Reconocer el parmatro DELETE
            else if (strncmp(this->toLower(this->cadena).c_str(),delete_param.c_str(),delete_param.length())==0){
                this->obtenerDatoParamS(particion->delete_, delete_param.length());
            }

                //Reconocer el parmatro NAME
            else if (strncmp(this->toLower(this->cadena).c_str(),name_param.c_str(),name_param.length())==0){
                this->obtenerDatoParamC(particion->name, name_param.length());
            }

                //Reconocer el parametro ADD
            else if (strncmp(this->toLower(this->cadena).c_str(),add_param.c_str(),add_param.length())==0){
                this->obtenerDatoParamN(particion->add, add_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        particion->fdisk();
    }

        //Comando Exec
    else if (tipoInst == 28) {
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string path_param = ">path=";
        string path = "";
        string nombre = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(path,nombre,path_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        if (this->verificarExtension(nombre)) {
            string fullpath = path + "/" + nombre;
            FILE *file = fopen(fullpath.c_str(), "r");
            if (file != NULL) {
                char comando[1024];
                while (fgets(comando, 1024, file)) {
                    strtok(comando, "\n");
                    string comandoS = comando;
                    comandoS = this->trim(comandoS);
                    if (comandoS.length() != 0) {
                        Analizador *analizador = new Analizador(comandoS);
                        analizador->analizar();
                    }
                }
                fclose(file);
            }
            else {
                cout << "No se encontro el archivo especificado" << endl;
                cout << endl;
            }
        }
    }

        //No se reconoce un comando
    else {
        cout << "El comando no pudo ser reconocido" << endl;
        cout << endl;
    }
}
