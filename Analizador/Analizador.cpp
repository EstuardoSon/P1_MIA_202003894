//
// Created by estuardo on 11/02/23.
//

#include "Analizador.h"
#include "../Estructuras y Objetos/Disco.h"
#include "../Estructuras y Objetos/Particion.h"
#include "../Estructuras y Objetos/Reporte.h"
#include "../Estructuras y Objetos/AdminUsuario.h"
#include "../Estructuras y Objetos/GestorArchivos.h"
#include <regex>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <string.h>

using namespace std;

Analizador::Analizador(std::string cadena, ListaMount *listamount, Usuario *usuario) {
    this->cadena = this->trim(cadena);
    this->listaMount = listaMount;
    this->usuario = usuario;
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

        string s_param = ">size=";
        string f_param = ">fit=";
        string u_param = ">unit=";
        string path_param = ">path=";

        Disco *disco = new Disco();

        while(this->cadena.length()>0){
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro S
            else if (strncmp(this->toLower(this->cadena).c_str(),s_param.c_str(),s_param.length())==0){
                this->obtenerDatoParamN(disco->s, s_param.length());
            }

                //Reconocer el parametro F
            else if (strncmp(this->toLower(this->cadena).c_str(),f_param.c_str(),f_param.length())==0){
                this->obtenerDatoParamS(disco->f, f_param.length());
            }

                //Reconocer el parmatro U
            else if (strncmp(this->toLower(this->cadena).c_str(),u_param.c_str(),u_param.length())==0){
                this->obtenerDatoParamS(disco->u, u_param.length());
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

        //Comando Mount
    else if(tipoInst == 4){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string path_param = ">path=";
        string name_param = ">name=";
        string id_param = "#id=";

        NodoMount * mount = new NodoMount();

        while(this->cadena.length() > 0) {
            //Reconocer el parmatro ID
            if (strncmp(this->toLower(this->cadena).c_str(),id_param.c_str(),id_param.length())==0){
                this->obtenerDatoParamC(mount->idCompleto, id_param.length());
            }

            else if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(mount->fichero, mount->nombre_disco,path_param.length());
            }

                //Reconocer el parmatro NAME
            else if (strncmp(this->toLower(this->cadena).c_str(),name_param.c_str(),name_param.length())==0){
                this->obtenerDatoParamC(mount->nombre_particion, name_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        listaMount->agregar(mount);
    }

        //Comando Unmount
    else if(tipoInst == 5){
        this->cadena = this->trim(this->cadena.erase(0, 7));

        string id_param = ">id=";
        string idCompleto = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parmatro ID
            else if (strncmp(this->toLower(this->cadena).c_str(),id_param.c_str(),id_param.length())==0){
                this->obtenerDatoParamC(idCompleto, id_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        listaMount->eliminar(idCompleto);
        listaMount->imprimirLista();
    }

        //Comando Mkfs
    else if(tipoInst == 6){
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string id_param = ">id=";
        string type_param = ">type=";
        string fs_param = ">fs=";
        string id = "";
        string type = "";
        string fs = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro ID
            else if (strncmp(this->toLower(this->cadena).c_str(), id_param.c_str(), id_param.length()) == 0) {
                this->obtenerDatoParamC(id, id_param.length());
            }

                //Reconocer el parametro TYPE
            else if (strncmp(this->toLower(this->cadena).c_str(), type_param.c_str(), type_param.length()) == 0) {
                this->obtenerDatoParamS(type, type_param.length());
            }

                //Reconocer el parametro FS
            else if (strncmp(this->toLower(this->cadena).c_str(), fs_param.c_str(), fs_param.length()) == 0) {
                this->obtenerDatoParamS(fs, fs_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        this->listaMount->mkfs(id, type, fs);
    }

        //Comando Login
    else if(tipoInst == 7){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string usr_param = ">user=";
        string pass_param = ">pass=";
        string id_param = ">id=";

        AdminUsuario * admin = new AdminUsuario(this->listaMount,this->usuario);
        string usuario = "";
        string password = "";
        string idParticion = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro USR
            else if (strncmp(this->toLower(this->cadena).c_str(), usr_param.c_str(), usr_param.length()) == 0) {
                this->obtenerDatoParamC(usuario, usr_param.length());
            }

                //Reconocer el parametro PASS
            else if (strncmp(this->toLower(this->cadena).c_str(), pass_param.c_str(), pass_param.length()) == 0) {
                this->obtenerDatoParamC(password, pass_param.length());
            }

                //Reconocer el parametro ID
            else if (strncmp(this->toLower(this->cadena).c_str(), id_param.c_str(), id_param.length()) == 0) {
                this->obtenerDatoParamC(idParticion, id_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
        admin->login(usuario,password,idParticion);
    }

        //Comando Logout
    else if(tipoInst == 8){
        this->cadena = this->trim(this->cadena.erase(0, 6));
        if(this->cadena != ""){
            cout << "Ingreso un parametro no reconocido" << endl;
            cout << endl;
            return;
        }
        AdminUsuario * admin = new AdminUsuario(this->listaMount, this->usuario);
        admin->logout();
    }

        //Comando Mkgrp
    else if(tipoInst == 9){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string name_param = ">name=";
        string name = "";
        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro Name
            else if (strncmp(this->toLower(this->cadena).c_str(), name_param.c_str(), name_param.length()) == 0) {
                this->obtenerDatoParamC(name, name_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        AdminUsuario * admin = new AdminUsuario(this->listaMount,this->usuario);
        admin->mkgrp(name);
    }

        //Comando Rmgrp
    else if(tipoInst == 10){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string name_param = ">name=";
        string name = "";
        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro Name
            else if (strncmp(this->toLower(this->cadena).c_str(), name_param.c_str(), name_param.length()) == 0) {
                this->obtenerDatoParamC(name, name_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        AdminUsuario * admin = new AdminUsuario(this->listaMount,this->usuario);
        admin->rmgrp(name);
    }

        //Comando Mkusr
    else if(tipoInst == 11){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string usr_param = ">user=";
        string pass_param = ">pass=";
        string grp_param = ">grp=";
        string usuario = "", password = "", grupo = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro usr
            else if (strncmp(this->toLower(this->cadena).c_str(), usr_param.c_str(), usr_param.length()) == 0) {
                this->obtenerDatoParamC(usuario, usr_param.length());
            }

                //Reconocer el parametro pass
            else if (strncmp(this->toLower(this->cadena).c_str(), pass_param.c_str(), pass_param.length()) == 0) {
                this->obtenerDatoParamC(password, pass_param.length());
            }

                //Reconocer el parametro grp
            else if (strncmp(this->toLower(this->cadena).c_str(), grp_param.c_str(), grp_param.length()) == 0) {
                this->obtenerDatoParamC(grupo, grp_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        AdminUsuario * admin = new AdminUsuario(this->listaMount,this->usuario);
        admin->mkusr(usuario, password, grupo);
    }

        //Comando Rmusr
    else if(tipoInst == 12){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string usr_param = ">user=";
        string usuario = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro usr
            else if (strncmp(this->toLower(this->cadena).c_str(), usr_param.c_str(), usr_param.length()) == 0) {
                this->obtenerDatoParamC(usuario, usr_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        AdminUsuario * admin = new AdminUsuario(this->listaMount,this->usuario);
        admin->rmusr(usuario);
    }

        //Comando Chmod
    else if(tipoInst == 13){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string path_param = ">path=";
        string ugo_param = ">ugo=";
        string r_param = ">r";
        string path = "";
        int ugo = 0;
        bool r = false;

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatoParamC(path, path_param.length());
            }

                //Reconocer el parametro UGO
            else if (strncmp(this->toLower(this->cadena).c_str(), ugo_param.c_str(), ugo_param.length()) == 0) {
                this->obtenerDatoParamN(ugo,ugo_param.length());
            }

                //Reconocer el parametro R
            else if (strncmp(this->toLower(this->cadena).c_str(), r_param.c_str(), r_param.length()) == 0) {
                r = true;
                this->cadena = this->trim(this->cadena.erase(0,r_param.length()));
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos * gestor = new GestorArchivos(listaMount, usuario);
        gestor->chmod(path,ugo,r);
    }

        //Comando Mkfile
    else if(tipoInst == 14){
        this->cadena = this->trim(this->cadena.erase(0, 6));

        string path_param = ">path=";
        string r_param = ">r";
        string s_param = ">size=";
        string cont_param = ">cont=";
        string path_fichero = "", path_archivo = "", cont_fichero = "", cont_archivo = "";
        int size = 0;
        bool r = false;

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(path_fichero,path_archivo,  path_param.length());
            }

                //Reconocer el parametro R
            else if (strncmp(this->toLower(this->cadena).c_str(), r_param.c_str(), r_param.length()) == 0) {
                r = true;
                this->cadena = this->trim(this->cadena.erase(0,r_param.length()));
            }

                //Reconocer el parametro S
            else if (strncmp(this->toLower(this->cadena).c_str(), s_param.c_str(), s_param.length()) == 0) {
                this->obtenerDatoParamN(size, s_param.length());
            }

                //Reconocer el parametro CONT
            else if (strncmp(this->toLower(this->cadena).c_str(), cont_param.c_str(), cont_param.length()) == 0) {
                this->obtenerDatosPath(cont_fichero,cont_archivo, cont_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos * gestor = new GestorArchivos(this->listaMount,this->usuario);
        gestor->mkfile(path_fichero, path_archivo,r,size,cont_fichero, cont_archivo);
    }

        //Comando Cat
    else if(tipoInst == 15){
        this->cadena = this->trim(this->cadena.erase(0, 3));

        regex regex_exp ("(>file)[0-9]+(=)");
        vector<string> rutas;

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

            int pos = this->cadena.find("=");
            string aux = this->cadena.substr(0,pos+1);

            //Reconocer el parametro FileN
            if (regex_match(aux, regex_exp)) {
                string ruta = "";
                this->obtenerDatoParamC(ruta,pos+1);
                rutas.push_back(ruta);
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos *gestos = new GestorArchivos(listaMount, usuario);
        gestos->cat(rutas);
    }

        //Comando Remove
    else if(tipoInst == 16){
        this->cadena = this->trim(this->cadena.erase(0, 6));

        string path_param = ">path=";
        string path = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatoParamC(path,path_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos *gestor = new GestorArchivos(listaMount,usuario);
        gestor->remove(path);
    }

        //Comando Edith
    else if(tipoInst == 17){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string path_param = ">path=";
        string cont_param = ">cont=";
        string path = "", cont = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatoParamC(path,path_param.length());
            }

                //Reconocer el parametro CONT
            else if (strncmp(this->toLower(this->cadena).c_str(), cont_param.c_str(), cont_param.length()) == 0) {
                this->obtenerDatoParamC(cont,  cont_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos *gestor = new GestorArchivos(listaMount, usuario);
        gestor->edit(path, cont);
    }

        //Comando Rename
    else if(tipoInst == 18){
        this->cadena = this->trim(this->cadena.erase(0, 6));

        string path_param = ">path=";
        string name_param = ">name=";
        string path = "", name = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatoParamC(path, path_param.length());
            }

                //Reconocer el parametro NAME
            else if (strncmp(this->toLower(this->cadena).c_str(), name_param.c_str(), name_param.length()) == 0) {
                this->obtenerDatoParamC(name, name_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos *gestor = new GestorArchivos(listaMount, usuario);
        gestor->rename(path, name);
    }

        //Comando Mkdir
    else if(tipoInst == 19){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string path_param = ">path=";
        string p_param = ">r";
        string path = "";
        bool p = false;

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatoParamC(path,path_param.length());
            }

                //Reconocer el parametro R
            else if (strncmp(this->toLower(this->cadena).c_str(), p_param.c_str(), p_param.length()) == 0) {
                p = true;
                this->cadena = this->trim(this->cadena.erase(0,p_param.length()));
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        GestorArchivos * gestor = new GestorArchivos(listaMount, usuario);
        gestor->mkdir(path,p);
    }

        //Comando Copy
    else if(tipoInst == 20){
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string path_param = ">path=";
        string destino_param = ">destino=";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->cadena = this->trim(this->cadena.erase(0,path_param.length()));
            }

                //Reconocer el parametro DESTINO
            else if (strncmp(this->toLower(this->cadena).c_str(), destino_param.c_str(), destino_param.length()) == 0) {
                this->cadena = this->trim(this->cadena.erase(0,destino_param.length()));
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
    }

        //Comando Move
    else if(tipoInst == 21){
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string path_param = ">path=";
        string destino_param = ">destino=";
        string path = "", destino = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
            }

                //Reconocer el parametro DESTINO
            else if (strncmp(this->toLower(this->cadena).c_str(), destino_param.c_str(), destino_param.length()) == 0) {
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
    }

        //Comando Find
    else if(tipoInst == 22){
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string path_param = ">path=";
        string name_param = ">name=";
        string path = "", name = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
            }

                //Reconocer el parametro NAME
            else if (strncmp(this->toLower(this->cadena).c_str(), name_param.c_str(), name_param.length()) == 0) {
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
    }

        //Comando Chown
    else if(tipoInst == 23){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string path_param = ">path=";
        string r_param = ">r";
        string usr_param = ">user=";
        string path = "", usr = "";
        bool r = false;

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro PATH
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
            }

                //Reconocer el parametro R
            else if (strncmp(this->toLower(this->cadena).c_str(), r_param.c_str(), r_param.length()) == 0) {
            }

                //Reconocer el parametro USR
            else if (strncmp(this->toLower(this->cadena).c_str(), usr_param.c_str(), usr_param.length()) == 0) {
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
    }

        //Comando Chgrp
    else if(tipoInst == 24){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        string usr_param = ">user=";
        string grp_param = ">grp=";
        string usr = "", grp = "";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro USUARIO
            else if (strncmp(this->toLower(this->cadena).c_str(), usr_param.c_str(), usr_param.length()) == 0) {
                this->obtenerDatoParamC( usr, usr_param.length());
            }


                //Reconocer el parametro GRUPO
            else if (strncmp(this->toLower(this->cadena).c_str(), grp_param.c_str(), grp_param.length()) == 0) {
                this->obtenerDatoParamC(grp, grp_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        AdminUsuario * admin = new AdminUsuario(this->listaMount, this->usuario);
        admin->chgrp(usr,grp);
    }

        //Comando Pause
    else if(tipoInst == 25){
        this->cadena = this->trim(this->cadena.erase(0, 5));

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)) {
                break;
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
        cout << "Precione cualquier tecla para continuar... " << endl;
        system("read continue");
    }

        //Comando Recovery
    else if(tipoInst == 26){
        this->cadena = this->trim(this->cadena.erase(0, 8));

        string id_param = ">id=";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro Id
            else if (strncmp(this->toLower(this->cadena).c_str(), id_param.c_str(), id_param.length()) == 0) {
                //this->obtenerDatoParamC(fichero, 5);
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
    }

        //Comando Loss
    else if(tipoInst == 27){
        this->cadena = this->trim(this->cadena.erase(0, 4));

        string id_param = ">id=";

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro Id
            else if (strncmp(this->toLower(this->cadena).c_str(), id_param.c_str(), id_param.length()) == 0) {
                //this->obtenerDatoParamC(fichero, 5);
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }
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
                        Analizador *analizador = new Analizador(comando, listaMount, usuario);
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

        //Comando Rep
    else if(tipoInst == 29){
        this->cadena = this->trim(this->cadena.erase(0, 3));

        string name_param = ">name=";
        string path_param = ">path=";
        string id_param = ">id=";
        string ruta_param = ">ruta=";

        Reporte * reporte = new Reporte(this->listaMount);

        while(this->cadena.length() > 0) {
            if (this->verificarComentario(this->cadena)){
                break;
            }

                //Reconocer el parametro Name
            else if (strncmp(this->toLower(this->cadena).c_str(), name_param.c_str(), name_param.length()) == 0) {
                this->obtenerDatoParamS(reporte->name, name_param.length());
            }

                //Reconocer el parametro Path
            else if (strncmp(this->toLower(this->cadena).c_str(), path_param.c_str(), path_param.length()) == 0) {
                this->obtenerDatosPath(reporte->fichero_p, reporte->archivo_p, path_param.length());
            }

                //Reconocer el parametro Id
            else if (strncmp(this->toLower(this->cadena).c_str(), id_param.c_str(), id_param.length()) == 0) {
                this->obtenerDatoParamC(reporte->id, id_param.length());
            }

                //Reconocer el parametro Ruta
            else if (strncmp(this->toLower(this->cadena).c_str(), ruta_param.c_str(), ruta_param.length()) == 0) {
                this->obtenerDatosPath(reporte->fichero_r, reporte->archivo_r, ruta_param.length());
            }

                //No se pudo reconocer el tipo de parametro
            else {
                cout << "Ingreso un parametro no reconocido" << endl;
                cout << endl;
                return;
            }
        }

        reporte->generarReporte();
    }

        //No se reconoce un comando
    else {
        cout << "El comando no pudo ser reconocido" << endl;
        cout << endl;
    }
}
