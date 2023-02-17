//
// Created by estuardo on 13/02/23.
//

#include "ListaMount.h"
#include <iostream>
#include <string.h>
#include <regex>
#include <cmath>

using namespace std;

ListaMount::ListaMount(){
    this->inicio = NULL;
    this->fin = NULL;
}

//Obtener tipo de formateo
int ListaMount::obtenerType(std::string type) {
    if(type == "fast"){
        return 1;
    }
    else if(type == "full" || type == ""){
        return 2;
    }
    cout << "Tipo de formateo no identificado" << endl << endl;
    return -1;
}

//Obtener tipo de sistema de archivos
int ListaMount::obtenerFs(std::string fs) {
    if(fs == "2fs" || fs == ""){
        return 1;
    }
    else if(fs == "3fs"){
        return 2;
    }
    cout << "Tipo de sistema de archivos no identificado" << endl << endl;
    return -1;
}

//Verificar la existencia de la parcticion entre Principales y Extendidas
bool ListaMount::verificarParticion(FILE *archivo, MBR &mbr, NodoMount * nuevo) {
    bool verificacion = false;
    for(int i=0; i<4; i++){
        if(strncmp(nuevo->nombre_particion.c_str(),mbr.mbr_partition_[i].part_name,16) == 0 && mbr.mbr_partition_[i].part_status != '0' && mbr.mbr_partition_[i].part_type !='E') {
            if(mbr.mbr_partition_[i].part_status == '2'){
                SuperBloque sb;
                fseek(archivo,mbr.mbr_partition_[i].part_start, SEEK_SET);
                fread(&sb, sizeof(SuperBloque), 1, archivo);

                sb.s_mtime = time(nullptr);
                sb.s_mnt_count++;
                fseek(archivo,mbr.mbr_partition_[i].part_start, SEEK_SET);
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }
            nuevo->part_start = mbr.mbr_partition_[i].part_start;
            nuevo->part_type = mbr.mbr_partition_[i].part_type;
            verificacion = true;
            break;
        }
        else if(strncmp(nuevo->nombre_particion.c_str(),mbr.mbr_partition_[i].part_name,16) == 0 && mbr.mbr_partition_[i].part_status == '0') {
            cout << "La Particion no puede montarse ya que fue eliminada FAST" << endl << endl;
            verificacion = false;
            break;
        }
        else if (mbr.mbr_partition_[i].part_type == 'E' && mbr.mbr_partition_[i].part_status == '1'){
            verificacion = this->verificarParticionL(archivo,mbr.mbr_partition_[i],nuevo);
            if(verificacion){
                break;
            }
        }
    }
    if(!verificacion) {
        cout << "No se encontro una Particion con el nombre establecido" << endl << endl;
    }
    return verificacion;
}

//Verificar la existencia de la particion entre Logicas
bool ListaMount::verificarParticionL(FILE *archivo, Partition & p, NodoMount * nuevo) {
    EBR ebr;
    fseek(archivo, p.part_start, SEEK_SET);
    fread(&ebr, sizeof(EBR), 1, archivo);

    if (strncmp(ebr.part_name,nuevo->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
        if(ebr.part_status == '2'){
            SuperBloque sb;
            fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
            fread(&sb, sizeof(SuperBloque), 1, archivo);

            sb.s_mtime = time(nullptr);
            sb.s_mnt_count++;
            fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
            fread(&sb, sizeof(SuperBloque), 1, archivo);
        }
        nuevo->part_type = 'L';
        return true;
    } else if(strncmp(nuevo->nombre_particion.c_str(),ebr.part_name,16) == 0 && ebr.part_status == '0') {
        cout << "La Particion no puede montarse ya que fue eliminada FAST" << endl << endl;
        return false;
    }
    while (ebr.part_next != -1) {
        fseek(archivo, ebr.part_next, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);

        if (strncmp(ebr.part_name,nuevo->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
            if(ebr.part_status == '2'){
                SuperBloque sb;
                fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                fread(&sb, sizeof(SuperBloque), 1, archivo);

                sb.s_mtime = time(nullptr);
                sb.s_mnt_count++;
                fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }
            nuevo->part_type = 'L';
            return true;
        }
        else if(strncmp(nuevo->nombre_particion.c_str(),ebr.part_name,16) == 0 && ebr.part_status == '0') {
            cout << "La Particion no puede montarse ya que fue eliminada FAST" << endl << endl;
            return false;
        }
    }
    return false;
}

//Verificar ID completo
bool ListaMount::verificarID(NodoMount *nuevo) {
    regex regex_exp ("(94)[0-9]+[a-zA-ZñÑ]+[0-9a-zA-ZñÑ]+");
    if(regex_match(nuevo->idCompleto, regex_exp)) {
        this->obtenerIdNumero(nuevo);
        return true;
    }
    cout << "El ID no cumple con los requisitos minimos 94 + Num + Letra" << endl << endl;
    return false;
}

//Verificar la Existencia del disco
bool ListaMount::verificarRuta(NodoMount * nuevo) {
    regex expresion(
            "(\\/[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+\\/)([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.dsk)");
    if (!regex_match(nuevo->fichero + "/" + nuevo->nombre_disco, expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return false;
    }

    FILE *archivo = fopen((nuevo->fichero + "/" + nuevo->nombre_disco).c_str(), "rb+");

    if (archivo != NULL) {
        MBR mbr;
        fseek(archivo, 0, SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        bool verificar = this->verificarParticion(archivo, mbr, nuevo);
        fclose(archivo);
        return verificar;
    } else {
        cout << "El archivo no se encontro en la ruta establecida" << endl << endl;
        return false;
    }
}

//Verificar que la particion no este actualmente montada
bool ListaMount::verificarLista(NodoMount *nuevo) {
    NodoMount * aux = this->inicio;
    while(aux != NULL){
        if(aux->idCompleto == nuevo->idCompleto){
            cout << "El ID ya esta en uso" << endl << endl;
            return false;
        } else if(aux->fichero == nuevo->fichero && aux->nombre_disco == nuevo->nombre_disco && aux->nombre_particion == nuevo->nombre_particion){
            cout << "La particion ya se encuentra montada" << endl << endl;
            return false;
        }
        aux = aux->next;
    }
    return true;
}

//Eliminar Nodo de la lista
NodoMount * ListaMount::eliminar(string idCompleto) {
    NodoMount *aux = this->inicio;
    NodoMount *ant = NULL;
    while (aux != NULL) {
        if (aux->idCompleto == idCompleto) {
            FILE * archivo = fopen((aux->fichero+"/"+aux->nombre_disco).c_str(),"rb+");
            if(archivo != NULL){
                if(aux->part_type == 'P'){
                    MBR mbr;
                    fseek(archivo,0,SEEK_SET);
                    fread(&mbr, sizeof(MBR), 1, archivo);

                    Partition p;
                    p.part_start = -1;
                    for(int i=0; i < 4; i++){
                        if(strncmp(mbr.mbr_partition_[i].part_name, aux->nombre_particion.c_str(), 16) == 0 &&
                           mbr.mbr_partition_[i].part_status != '0' && mbr.mbr_partition_[i].part_type == 'P'){
                            p = mbr.mbr_partition_[i];
                        }
                    }

                    if(p.part_start != -1){
                        if(p.part_status == '2'){
                            SuperBloque sb;
                            fseek(archivo, p.part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivo);
                            sb.s_umtime = time(nullptr);
                            fseek(archivo, p.part_start, SEEK_SET);
                            fwrite(&sb, sizeof(SuperBloque), 1, archivo);
                        }
                    }
                }
                else if(aux->part_type == 'E'){
                    EBR ebr;
                    SuperBloque sb;
                    fseek(archivo, aux->part_start, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);

                    if (strncmp(ebr.part_name,aux->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
                        if(ebr.part_status == '2'){

                            fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivo);
                            sb.s_umtime = time(nullptr);
                            fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivo);
                        }

                    }
                    while (ebr.part_next != -1) {
                        fseek(archivo, ebr.part_next, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivo);

                        if (strncmp(ebr.part_name,aux->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
                            if(ebr.part_status == '2'){
                                fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivo);
                                sb.s_mtime = time(nullptr);
                                fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivo);
                            }
                        }
                    }
                }
            }
            fclose(archivo);

            cout << "Id: " << aux->idCompleto << " Nombre: " << aux->id << " Numero: " << aux->numero << endl;
            cout << "Montura Eliminada... " << endl << endl;
            if (ant != NULL) {
                if (this->inicio == aux) {
                    this->inicio = aux->next;
                }
                if (this->fin == aux) {
                    this->fin = ant;
                }
                ant->next = aux->next;
                aux->next = NULL;
                return aux;
            } else {
                if (this->inicio == aux) {
                    this->inicio = aux->next;
                }
                if (this->fin == aux) {
                    this->fin = ant;
                }
                return aux;
            }
        }
        ant = aux;
        aux = aux->next;
    }
    cout << "No se encontro una Montura con dicho ID" << endl << endl;
    return NULL;
}

//Buscar Nodo en la lista
NodoMount *ListaMount::buscar(string idCompleto) {
    NodoMount *aux = this->inicio;
    while (aux != NULL) {
        if (aux->idCompleto == idCompleto) {
            return aux;
        }
        aux = aux->next;
    }
    cout << "No se encontro una Montura con dicho ID" << endl << endl;
    return NULL;
}

//Obtener el numero y nombre del ID
void ListaMount::obtenerIdNumero(NodoMount *nuevo) {
    string valor = nuevo->idCompleto.substr(2);
    string numero = "";

    int i;
    for(i = 0; i < valor.length(); i++){
        if(((char)valor.c_str()[i]) >= 48 && ((char)valor.c_str()[i]) <= 57){
            numero += valor.c_str()[i];
        } else {
            break;
        }
    }

    nuevo->id = valor.substr(i);
    nuevo->numero = stoi(valor);
}

//Agregar Nodos a la lista
void ListaMount::agregar(NodoMount * nuevo) {
    if (this->verificarID(nuevo)) {
        if (this->verificarLista(nuevo)) {
            if (this->verificarRuta(nuevo)) {
                if (nuevo->part_type != 'E') {
                    if (this->inicio == NULL) {
                        this->inicio = nuevo;
                        this->fin = nuevo;
                    } else {
                        this->fin->next = nuevo;
                        this->fin = nuevo;
                    }

                    FILE * archivo = fopen((nuevo->fichero+"/"+nuevo->nombre_disco).c_str(),"rb+");
                    if(archivo != NULL){
                        if(nuevo->part_type == 'P'){
                            MBR mbr;
                            fseek(archivo,0,SEEK_SET);
                            fread(&mbr, sizeof(MBR), 1, archivo);

                            Partition p;
                            p.part_start = -1;
                            for(int i=0; i < 4; i++){
                                if(strncmp(mbr.mbr_partition_[i].part_name, nuevo->nombre_particion.c_str(), 16) == 0 &&
                                   mbr.mbr_partition_[i].part_status != '0' && mbr.mbr_partition_[i].part_type == 'P'){
                                    p = mbr.mbr_partition_[i];
                                }
                            }

                            if(p.part_start != -1){
                                if(p.part_status == '2'){
                                    SuperBloque sb;
                                    fseek(archivo, p.part_start, SEEK_SET);
                                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                                    sb.s_mtime = time(nullptr);
                                    sb.s_mnt_count++;
                                    fseek(archivo, p.part_start, SEEK_SET);
                                    fwrite(&sb, sizeof(SuperBloque), 1, archivo);
                                }
                            }
                        }
                        else if(nuevo->part_type == 'E'){
                            EBR ebr;
                            SuperBloque sb;
                            fseek(archivo, nuevo->part_start, SEEK_SET);
                            fread(&ebr, sizeof(EBR), 1, archivo);

                            if (strncmp(ebr.part_name,nuevo->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
                                if(ebr.part_status == '2'){

                                    fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                                    sb.s_mtime = time(nullptr);
                                    sb.s_mnt_count++;
                                    fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                                }

                            }
                            while (ebr.part_next != -1) {
                                fseek(archivo, ebr.part_next, SEEK_SET);
                                fread(&ebr, sizeof(EBR), 1, archivo);

                                if (strncmp(ebr.part_name,nuevo->nombre_particion.c_str(),16) == 0 && ebr.part_status != '0') {
                                    if(ebr.part_status == '2'){
                                        fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                        fread(&sb, sizeof(SuperBloque), 1, archivo);
                                        sb.s_mtime = time(nullptr);
                                        sb.s_mnt_count++;
                                        fseek(archivo,ebr.part_start + sizeof(EBR), SEEK_SET);
                                        fread(&sb, sizeof(SuperBloque), 1, archivo);
                                    }
                                }
                            }
                        }
                    }
                    fclose(archivo);

                    cout << "Id: " << nuevo->idCompleto << " Nombre: " << nuevo->id << " Numero: " << nuevo->numero
                         << endl;
                    cout << "Montura Realizada..." << endl << endl;

                    cout << "Monturas Actuales"<< endl;
                    this->imprimirLista();
                }
                else{
                    cout << "No es posible montar una Particion Extendida..." << endl << endl;
                }
            }
        }
    }
}

//Imprimir la lista
void ListaMount::imprimirLista() {
    NodoMount * aux = this->inicio;

    while(aux != NULL){
        cout << "Id: " << aux->idCompleto << " Nombre: " << aux->id << " Numero: " << aux->numero << endl;
        aux = aux->next;
    }
    cout << endl;
}