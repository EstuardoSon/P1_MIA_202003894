//
// Created by estuardo on 13/02/23.
//

#include "Particion.h"

#include "Estructura.h"
#include <iostream>
#include <cstring>
#include <regex>

using namespace std;

Particion::Particion() {
    this->s = -1;
    this->u = "";
    this->path = "";
    this->nombre_disco = "";
    this->t = "";
    this->f = "";
    this->delete_ = "";
    this->name = "";
    this->add = 0;
}

//Devuelve un numero de acuerdo al tipo d eunidades de la particion
int Particion::verificarTamanio() {
    if (this->u == "b"){ return 1; }
    else if (this->u == "k" || this->u == ""){ return 1024; }
    else if (this->u == "m"){ return 1024 * 1024; }
    return -1;
}

//Devuelve un numero de acuerdo al tipo de particion
int Particion::verificarTipoP() {
    if (this->t == "p" ||  this->t == ""){ return 1; }
    else if (this->t == "e"){ return 2; }
    else if (this->t == "l"){ return 3; }
    return -1;
}

//Devuelve un numero deacuerdo al tipo de ajuste
int Particion::verificarAjusteP() {
    if (this->f == "bestfit"){ return 1; }
    else if (this->f == "firstfit"){ return 2; }
    else if (this->f == "worstfit" || this->f == ""){ return 3; }
    return -1;
}

//Obtener la posicion de inicio de la particion
int Particion::obtenerInicioP() {
    FILE * archivo = fopen((this->path+"/"+this->nombre_disco).c_str(), "rb+");
    MBR mbr;
    fread(&mbr, sizeof(MBR),1, archivo);
    fclose(archivo);

    if (mbr.mbr_partition_[0].part_start == -1){
        return sizeof(MBR);
    }
    for(int i = 1; i<4; i++){
        if(mbr.mbr_partition_[i].part_start == -1){
            return mbr.mbr_partition_[i-1].part_start + mbr.mbr_partition_[i-1].part_s;
        }
    }
}

//Obtener el inicio de la particion I/E siguiente
int Particion::obtenerInincioSiguiente(MBR &mbr, int posParticion) {
    for (int i = posParticion; i < 3; i++) {
        if (mbr.mbr_partition_[i + 1].part_start != -1) { return mbr.mbr_partition_[i + 1].part_start; }
    }
    return mbr.mbr_tamano;
}

//Obtener la cantidad de bits libres despues de la particion
int Particion::obtenerSizeDP(MBR &mbr) {
    for(int i=0; i<4 ; i++){
        if (strncmp(mbr.mbr_partition_[i].part_name, this->name.c_str(), 16) == 0){
            return this->obtenerInincioSiguiente(mbr,i) - mbr.mbr_partition_[i].part_start - mbr.mbr_partition_[i].part_s;
        }
    }
    return mbr.mbr_tamano - sizeof(MBR);
}

//Buscar Particion Extendida
Partition Particion::buscarParticionE(MBR &mbr) {
    for (int i = 0; i < 4; i++){
        if (mbr.mbr_partition_[i].part_type == 'E' && mbr.mbr_partition_[i].part_status != '0'){
            return mbr.mbr_partition_[i];
        }
    }
    cout << "No existe una Particion Extendida dentro del disco" << endl << endl;
    Partition p;
    p.part_status = '0';
    return p;
}

//Crear Struct Partition
Partition Particion::crearStructPartition(char tipo) {
    Partition p;
    p.part_status = '1';
    p.part_type = tipo;
    if (this->verificarAjusteP() == 1) { p.part_fit = 'B'; }
    else if (this->verificarAjusteP() == 2) { p.part_fit = 'F'; }
    else if (this->verificarAjusteP() == 3) { p.part_fit = 'W'; }
    p.part_start = this->obtenerInicioP();
    p.part_s = this->s;
    stpcpy(p.part_name, this->name.c_str());
    this->mostrarDatosP(p);
    return p;
}

//Verificar nombre en particiones
bool Particion::verificarNombreP(MBR &mbr, char &tipo) {
    bool verificacion = false;
    for (int i=0; i<4; i++){
        if (strncmp(mbr.mbr_partition_[i].part_name,this->name.c_str(),16) == 0){
            tipo = mbr.mbr_partition_[i].part_type;
            verificacion = true;
            break;
        }
        else if (mbr.mbr_partition_[i].part_type == 'E'){
            verificacion = this->verificarNombrePL(mbr.mbr_partition_[i], tipo);
            if(verificacion){
                break;
            }
        }
    }
    return verificacion;
}

//Verificar el nombre en particiones logicas
bool Particion::verificarNombrePL(Partition &partition, char &tipo) {
    EBR ebr;
    string fullpath = this->path + "/" + this->nombre_disco;
    FILE *file = fopen(fullpath.c_str(), "rb+");
    fseek(file, partition.part_start, SEEK_SET);
    fread(&ebr, sizeof(EBR), 1, file);

    if (strncmp(ebr.part_name,this->name.c_str(),16) == 0) {
        fclose(file);
        tipo = 'L';
        return true;
    }
    while (ebr.part_next != -1) {
        fseek(file, ebr.part_next, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, file);

        if (strncmp(ebr.part_name,this->name.c_str(),16) == 0) {
            fclose(file);
            tipo = 'L';
            return true;
        }
    }
    fclose(file);
    return false;
}

//Verificar que exista espacio en el disco
bool Particion::verificarEspacio(string path, int tipoP) {
    FILE * archivo = fopen(path.c_str(),"rb+");
    MBR mbr;
    fseek(archivo, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    if (tipoP == 2) {
        this->s = this->s + sizeof(EBR);
    }

    int espacioD = mbr.mbr_tamano - sizeof(MBR);
    if (mbr.mbr_partition_[0].part_start == -1) {
        espacioD = this->obtenerInincioSiguiente(mbr, 0) - sizeof(MBR);
    } else{
        for(int i = 1; i < 4; i++){
            if (mbr.mbr_partition_[i].part_start == -1) {
                espacioD = this->obtenerInincioSiguiente(mbr, i) - mbr.mbr_partition_[i].part_start - mbr.mbr_partition_[i].part_s;
                break;
            }
        }
    }
    fclose(archivo);
    cout << "Espacio Disponible: " << espacioD << endl ;
    cout << "Espacio Requerido: " << this->s << endl << endl ;
    return espacioD >= this->s;
}

//Verificar que exista espacio en una partida Extendida
bool Particion::verificarEspacioPE(Partition &p, EBR &nextEBR) {
    FILE *archivo = fopen((this->path+"/"+this->nombre_disco).c_str(),"rb+");
    EBR ebrAux;
    fseek(archivo, p.part_start, SEEK_SET);
    fread(&ebrAux, sizeof(EBR), 1, archivo);

    while(ebrAux.part_next != -1){
        fseek(archivo, ebrAux.part_next, SEEK_SET);
        fread(&ebrAux, sizeof(EBR), 1, archivo);
    }

    nextEBR.part_next = -1;
    nextEBR.part_start = ebrAux.part_start + ebrAux.part_s;
    stpcpy(nextEBR.part_name, this->name.c_str());
    this->s = this->s + sizeof(EBR);
    nextEBR.part_s = this->s;
    if (this->verificarAjusteP() == 1) { nextEBR.part_fit = 'B'; }
    else if (this->verificarAjusteP() == 2) { nextEBR.part_fit = 'F'; }
    else if (this->verificarAjusteP() == 3) { nextEBR.part_fit = 'W'; }
    nextEBR.part_status = '1';

    int espacioD = p.part_start + p.part_s - ebrAux.part_start - ebrAux.part_s;
    cout << "Espacio D. en PLogica: " << espacioD << endl;
    cout << "Espacio Requerido: " << this->s << endl << endl;

    if (p.part_s >= this->s) {
        ebrAux.part_next = nextEBR.part_start;
        fseek(archivo, ebrAux.part_start, SEEK_SET);
        fwrite(&ebrAux, sizeof(EBR), 1, archivo);
        fclose(archivo);
        return true;
    }else{
        fclose(archivo);
        return false;
    }
}

//Verificar que no exitan otra partidas extendidas
bool Particion::verificarPE(MBR &mbr) {
    if ((mbr.mbr_partition_[0].part_type == 'E' && mbr.mbr_partition_[0].part_status != '0') || (mbr.mbr_partition_[1].part_type == 'E' && mbr.mbr_partition_[1].part_status != '0') || (mbr.mbr_partition_[2].part_type == 'E' && mbr.mbr_partition_[2].part_status != '0') || (mbr.mbr_partition_[3].part_type == 'E' && mbr.mbr_partition_[3].part_status != '0')) {
        cout << "Error ya existe una Particion Extendida" << endl << endl;
        return true;
    } else{
        return false;
    }
}

//Mostrar datos de una particion
void Particion::mostrarDatosP(Partition &p) {
    cout << "- Particion Creada con Exito -" << endl;
    cout << "Nombre:" << p.part_name << endl;
    cout << "Estatus:" << p.part_status << endl;
    cout << "Tipo:" << p.part_type << endl;
    cout << "Ajuste:" << p.part_fit << endl;
    cout << "Inicio:" << p.part_start << endl;
    cout << "Size:" << p.part_s << endl << endl;
}

//Mostrar datos de un EBR
void Particion::mostrarDatosEBR(EBR &ebr) {
    cout << "- Particion Logica Creada con Exito -" << endl;
    cout << "Nombre:" << ebr.part_name << endl;
    cout << "Estatus:" << ebr.part_status << endl;
    cout << "Ajuste:" << ebr.part_fit << endl;
    cout << "Inicio:" << ebr.part_start << endl;
    cout << "Size:" << ebr.part_s << endl << endl;
}

void Particion::fdisk() {
    int tamanio = this->verificarTamanio();
    int ajusteP = this->verificarAjusteP();
    int tipoP = this->verificarTipoP();

    //Verificacion de los parametros S | T | F | DELETE
    if (tamanio == -1 || ajusteP == -1 || tipoP == -1 || (this->delete_ != "" && this->delete_ != "full" && this->delete_ != "fast") || this->name.length() == 0){
        cout << "No fue posible ejecutar el comando con la informacion proporcionada" << endl << endl;
        return;
    }

    regex expresion(
            "(\\/)([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.dsk)");
    if (!regex_match(this->path+"/"+this->nombre_disco,expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return;
    }

    FILE *archivo = fopen((this->path + "/" + this->nombre_disco).c_str(),"rb+");
    if(archivo != NULL){
        MBR mbr;
        fread(&mbr, sizeof(MBR), 1, archivo);
        char tipoParticion = '\0';

        //Eliminar Particion
        if(this->delete_ == "full"){
            if(this->verificarNombreP(mbr,tipoParticion)) {
                cout << "La particion fue encontrada, seguro que quiere eliminarla [Y/n]" << endl;
                cout << ">>";
                string opcion;
                getline(cin, opcion);
                if(opcion == "Y" || opcion == "y"){
                    char vacio = '\0';

                    //Eliminar Particiones Principales y Extendidas
                    if(tipoParticion == 'P' || tipoParticion == 'E'){
                        Partition p;
                        p.part_status = '0';
                        p.part_start = -1;
                        p.part_s = 0;
                        for(int i = 0; i < 16; i++){ p.part_name[i] = '\0'; }
                        for(int x=0; x < 4; x++){
                            if(strncmp(mbr.mbr_partition_[x].part_name,this->name.c_str(),16) == 0){
                                fseek(archivo,mbr.mbr_partition_[x].part_start,SEEK_SET);
                                for(int i = 0; i < mbr.mbr_partition_[x].part_s; i++){
                                    fwrite(&vacio, sizeof(char),1,archivo);
                                }
                                mbr.mbr_partition_[x] = p;
                                break;
                            }
                        }

                        fseek(archivo,0,SEEK_SET);
                        fwrite(&mbr, sizeof(MBR),1,archivo);
                    }

                        //Eliminar Particiones Logicas
                    else if(tipoParticion == 'L'){
                        Partition pAux = this->buscarParticionE(mbr);
                        EBR ebr;
                        fseek(archivo, pAux.part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivo);

                        EBR aux;
                        aux.part_status = '0';
                        aux.part_start = -1;
                        aux.part_next = ebr.part_next;
                        for(int n = 0; n < 16; n++){ aux.part_name[n] = '\0'; }
                        aux.part_s = ebr.part_s;
                        if(strncmp(ebr.part_name, this->name.c_str(),16) == 0){
                            fseek(archivo, pAux.part_start, SEEK_SET);
                            fwrite(&aux, sizeof(EBR), 1, archivo);
                            for(int i = 0; i < ebr.part_s - sizeof(EBR); i++){
                                fwrite(&vacio, sizeof(char), 1, archivo);
                            }
                        }else{
                            while(ebr.part_next != -1){
                                fseek(archivo, ebr.part_next, SEEK_SET);
                                fread(&aux, sizeof(EBR), 1, archivo);
                                if(strncmp(aux.part_name, this->name.c_str(),16) == 0){
                                    fseek(archivo, ebr.part_start, SEEK_SET);
                                    ebr.part_next = aux.part_next;
                                    fwrite(&ebr, sizeof(EBR), 1, archivo);
                                    fseek(archivo, aux.part_start, SEEK_SET);
                                    for(int i = 0; i < aux.part_s; i++){
                                        fwrite(&vacio, sizeof(char), 1, archivo);
                                    }
                                }
                                ebr = aux;
                            }
                        }
                    }
                    cout << "Particion Eliminada FULL" << endl << endl;
                }else{
                    cout << "Cancelando eliminacion..." << endl << endl;
                }
            } else {
                cout << "No se encontro ninguna Particion con el nombre indicado" << endl <<endl;
            }
        }

        else if(this->delete_ == "fast"){
            if(this->verificarNombreP(mbr,tipoParticion)) {
                cout << "La particion fue encontrada, seguro que quiere eliminarla [Y/n]" << endl;
                cout << ">>";
                string opcion;
                getline(cin, opcion);
                if(opcion == "Y" || opcion == "y"){
                    char vacio = '\0';

                    //Eliminar Particiones Principales y Extendidas
                    if(tipoParticion == 'P' || tipoParticion == 'E'){
                        for(int i=0; i < 4; i++) {
                            if (strncmp(mbr.mbr_partition_[i].part_name, this->name.c_str(), 16) ==
                                0) {
                                if (mbr.mbr_partition_[i].part_status != '0') {
                                    mbr.mbr_partition_[i].part_status = '0';
                                    fseek(archivo,0,SEEK_SET);
                                    fwrite(&mbr, sizeof(MBR),1,archivo);
                                    return;
                                }
                            }
                        }
                        cout << "La particion ya fue borrada FAST anteriormente" << endl << endl;
                        return;
                    }

                        //Eliminar Particiones Logicas
                    else if(tipoParticion == 'L') {
                        Partition pAux = this->buscarParticionE(mbr);
                        EBR ebr;
                        fseek(archivo, pAux.part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivo);

                        ebr.part_status = '0';
                        if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0 &&
                            ebr.part_status != '0') {
                            fseek(archivo, pAux.part_start, SEEK_SET);
                            fwrite(&ebr, sizeof(EBR), 1, archivo);
                        }else{
                            cout << "La particion ya fue borrada FAST anteriormente" << endl << endl;
                            return;
                        }
                        while (ebr.part_next != -1) {
                            fseek(archivo, ebr.part_next, SEEK_SET);
                            fread(&ebr, sizeof(EBR), 1, archivo);
                            if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0 &&
                                ebr.part_status != '0') {
                                ebr.part_status = '0';
                                fseek(archivo, ebr.part_start, SEEK_SET);
                                fwrite(&ebr, sizeof(EBR), 1, archivo);
                                break;
                            } else {
                                cout << "La particion ya fue borrada FAST anteriormente" << endl << endl;
                                break;
                            }
                        }
                    }
                    cout << "Particion Eliminada FAST" << endl << endl;
                }else{
                    cout << "Cancelando eliminacion..." << endl << endl;
                }
            } else {
                cout << "No se encontro ninguna Particion con el nombre indicado" << endl <<endl;
            }
        }

            //Add espacio
        else if(this->add != 0){
            if(this->verificarNombreP(mbr, tipoParticion)){
                this->add = this->add * tamanio;
                cout << "Espacio Solicitado: " << this->add << endl;

                //Reducir Espacio
                if(this->add < 0){
                    if(tipoParticion == 'P'){
                        for(int i =0 ; i < 4; i++){
                            if(strncmp(this->name.c_str(),mbr.mbr_partition_[i].part_name,16) == 0){
                                cout << "Espacio Actual de la Particion: " << mbr.mbr_partition_[i].part_s << endl;
                                if(mbr.mbr_partition_[i].part_s + this->add > 0 ){
                                    mbr.mbr_partition_[i].part_s = mbr.mbr_partition_[i].part_s + this->add;
                                    fseek(archivo, 0, SEEK_SET);
                                    fwrite(&mbr, sizeof(MBR), 1, archivo);
                                    cout << "Se redujo exitosamente la particion a: " << mbr.mbr_partition_[i].part_s << endl << endl;
                                    break;
                                } else {
                                    cout << "No se pudo reducir la particion" << endl << endl;
                                    break;
                                }
                            }
                        }
                    }
                    else if(tipoParticion == 'E'){
                        EBR ebr;
                        for(int i = 0 ; i < 4 ; i++){
                            if(strncmp(this->name.c_str(),mbr.mbr_partition_[i].part_name,16) == 0){
                                fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                                fread(&ebr, sizeof(EBR), 1, archivo);

                                int espacioNU = mbr.mbr_partition_[i].part_s - sizeof(EBR);
                                while(ebr.part_next != -1){
                                    fseek(archivo, ebr.part_next, SEEK_SET);
                                    fread(&ebr, sizeof(EBR), 1, archivo);
                                    espacioNU = mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s - ebr.part_start - ebr.part_s;
                                }

                                cout << "Espacio Actual de la Particion: " << mbr.mbr_partition_[i].part_s << endl;
                                cout << "Espacio No Usado: " << espacioNU << endl;

                                if(espacioNU + this->add > 0 ){
                                    mbr.mbr_partition_[i].part_s = mbr.mbr_partition_[i].part_s + this->add;
                                    fseek(archivo, 0, SEEK_SET);
                                    fwrite(&mbr, sizeof(MBR), 1, archivo);
                                    cout << "Se redujo exitosamente la particion a: " << mbr.mbr_partition_[i].part_s << endl << endl;
                                } else {
                                    cout << "No se pudo reducir la particion" << endl << endl;
                                }
                                break;
                            }
                        }
                    }
                    else if(tipoParticion == 'L'){
                        Partition p = this->buscarParticionE(mbr);
                        fseek(archivo, p.part_start, SEEK_SET);

                        EBR ebr;
                        fread(&ebr, sizeof(EBR), 1, archivo);

                        if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0){
                            cout << "Espacio Actual: " << ebr.part_s - sizeof(EBR) << endl;
                            int valor = ebr.part_s - sizeof(EBR) + this->add;
                            if(valor > 0){
                                ebr.part_s = ebr.part_s + this->add;
                                fseek(archivo, ebr.part_start, SEEK_SET);
                                fwrite(&ebr, sizeof(EBR), 1, archivo);
                                cout << "Se redujo exitosamente la particion a: " << ebr.part_s - sizeof(EBR) << endl << endl;
                            } else {
                                cout << "No se pudo reducir la particion" << endl << endl;
                            }
                        }
                        else {
                            while (ebr.part_next != -1) {
                                fseek(archivo, ebr.part_next, SEEK_SET);
                                fread(&ebr, sizeof(EBR), 1, archivo);

                                if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0) {
                                    cout << "Espacio Actual: " << ebr.part_s - sizeof(EBR) << endl;

                                    int valor = ebr.part_s - sizeof(EBR) + this->add;
                                    if(valor > 0){
                                        ebr.part_s = ebr.part_s + this->add;
                                        fseek(archivo, ebr.part_start, SEEK_SET);
                                        fwrite(&ebr, sizeof(EBR), 1, archivo);
                                        cout << "Se redujo exitosamente la particion a: " << ebr.part_s - sizeof(EBR) << endl << endl;
                                    } else {
                                        cout << "No se pudo reducir la particion" << endl << endl;
                                    }
                                }
                            }
                        }
                    }
                }

                    //Agregar Espacio
                else{
                    //Particion Principal y Extendida
                    if(tipoParticion == 'P' || tipoParticion == 'E'){
                        int sizeDP = this->obtenerSizeDP(mbr);
                        cout << "Espacio D. para Ampliacion: " << sizeDP << endl ;

                        if(sizeDP >= this->add){
                            for(int i = 0 ; i < 4; i++){
                                if(strncmp(this->name.c_str(),mbr.mbr_partition_[i].part_name,16) == 0){
                                    cout << "Espacio Actual de la particion: " << mbr.mbr_partition_[i].part_s << endl;
                                    mbr.mbr_partition_[i].part_s = mbr.mbr_partition_[i].part_s + this->add;
                                    cout << "Espacio de la Particion Ampliado Exitosamente a: " << mbr.mbr_partition_[i].part_s << endl << endl;
                                    break;
                                }
                            }

                            fseek(archivo,0,SEEK_SET);
                            fwrite(&mbr, sizeof(MBR),1, archivo);
                        } else {
                            cout << "No hay espacio suficiente para ampliar la Particion" << endl << endl;
                        }
                    }

                        //Particion Logica
                    else if(tipoParticion == 'L'){
                        Partition p = this->buscarParticionE(mbr);
                        fseek(archivo, p.part_start, SEEK_SET);
                        int limite = p.part_start + p.part_s;

                        EBR ebr, aux;
                        aux.part_next = -1;
                        aux.part_start = -1;
                        fread(&ebr, sizeof(EBR), 1, archivo);
                        if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0){
                            if (ebr.part_next != -1){
                                fseek(archivo, ebr.part_next,SEEK_SET);
                                fread(&aux, sizeof(EBR), 1, archivo);
                                limite = aux.part_start - ebr.part_start - ebr.part_s;
                            }
                        } else{
                            while (ebr.part_next != -1){
                                fseek(archivo, ebr.part_next,SEEK_SET);
                                fread(&ebr, sizeof(EBR), 1, archivo);

                                if (strncmp(ebr.part_name, this->name.c_str(), 16) == 0){
                                    if (ebr.part_next != -1){
                                        fseek(archivo, ebr.part_next,SEEK_SET);
                                        fread(&aux, sizeof(EBR), 1, archivo);
                                        limite = aux.part_start - ebr.part_start - ebr.part_s;
                                    }
                                }
                            }
                        }

                        cout << "Espacio D. para Ampliacion: " << limite << endl;
                        if(limite >= this->add){
                            cout << "Particion Size: "<< ebr.part_s << endl;
                            ebr.part_s = ebr.part_s + this->add;
                            fseek(archivo, ebr.part_start, SEEK_SET);
                            fwrite(&ebr, sizeof(EBR), 1, archivo);
                            cout << "Particion Ampliada con Exito a : "<< ebr.part_s << endl << endl;
                        } else {
                            cout << "No hay espacio suficiente para ampliar la Particion" << endl << endl;
                        }
                    }
                }
            } else{
                cout << "No se encontro una particion con el nombre establecido" << endl << endl;
            }
        }

            //Crear Particion
        else if(this->s >= 0){
            if(!(this->verificarNombreP(mbr, tipoParticion)) && this->name.length() <= 16) {
                this->s = this->s * tamanio;

                //Particiones Principales y Extendidas
                if (tipoP == 1 || tipoP == 2) {
                    if (this->verificarEspacio(this->path + "/" + this->nombre_disco, tipoP)) {

                        //Particion Primaria
                        if (tipoP == 1) {
                            for (int i = 0; i < 4; i++) {
                                if (mbr.mbr_partition_[i].part_start == -1) {
                                    mbr.mbr_partition_[i] = this->crearStructPartition('P');
                                    fseek(archivo, 0, SEEK_SET);
                                    fwrite(&mbr, sizeof(MBR), 1, archivo);
                                    fclose(archivo);
                                    return;
                                }
                            }
                            cout << "Actualmente ya existen 4 particiones" << endl << endl;
                            fclose(archivo);
                            return;
                        }

                            //Particion Extendida
                        else if (tipoP == 2) {
                            if (!this->verificarPE(mbr)) {
                                EBR ebr;
                                for (int x = 0; x < 4; x++) {
                                    if (mbr.mbr_partition_[x].part_start == -1) {
                                        mbr.mbr_partition_[x] = this->crearStructPartition('E');
                                        ebr.part_start = mbr.mbr_partition_[x].part_start;
                                        ebr.part_next = -1;
                                        ebr.part_status = '0';
                                        ebr.part_s = 0;
                                        for (int i = 0; i < sizeof(ebr.part_name) / sizeof(ebr.part_name[0]); i++) {
                                            ebr.part_name[i] = '\0';
                                        }
                                        fseek(archivo, 0, SEEK_SET);
                                        fwrite(&mbr, sizeof(MBR), 1, archivo);
                                        fseek(archivo, ebr.part_start, SEEK_SET);
                                        fwrite(&ebr, sizeof(EBR), 1, archivo);
                                        fclose(archivo);
                                        return;
                                    }
                                }
                                cout << "Actualmente ya existen 4 particiones" << endl << endl;
                                fclose(archivo);
                                return;
                            }
                        }
                    } else {
                        cout << "La particion excede el espacio disponible" << endl << endl;
                    }
                }

                    //Particion Logica
                else if(tipoP == 3){
                    Partition particionE = this->buscarParticionE(mbr);
                    if (particionE.part_status != '0'){
                        EBR ebr;
                        if(this->verificarEspacioPE(particionE, ebr)){
                            fseek(archivo, 0, SEEK_SET);
                            fwrite(&mbr, sizeof(MBR), 1, archivo);
                            fseek(archivo, ebr.part_start,SEEK_SET);
                            fwrite(&ebr, sizeof(EBR),1, archivo);
                            this->mostrarDatosEBR(ebr);
                        }else{
                            cout << "La particion logica excede el espacio disponible en la Particion Extendida" << endl << endl;
                        }
                    }
                }

            } else{
                cout << "El nombre de la particion ya esta en uso" << endl <<endl;
            }
        } else{
            cout << "El valor de tamanio para crear la Particion debe ser positivo diferente de 0" << endl;
            cout << "O el valor Add debe ser diferente de 0" << endl << endl;
        }
        fclose(archivo);
    } else{
        cout << "El archivo no fue encontrado en la direccion establecida" << endl << endl;
        return;
    }
}