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
                            fwrite(&sb, sizeof(SuperBloque), 1, archivo);
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
                                fwrite(&sb, sizeof(SuperBloque), 1, archivo);
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
                                    fwrite(&sb, sizeof(SuperBloque), 1, archivo);
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
                                        fwrite(&sb, sizeof(SuperBloque), 1, archivo);
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

//EXT2 y EXT3
void ListaMount::ext(int type, int sistemaArchivo, NodoMount * nodo) {
    MBR mbr;
    FILE * archivo = fopen((nodo->fichero+"/"+nodo->nombre_disco).c_str(), "rb+");
    if(archivo != NULL) {
        fseek(archivo, 0, SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);

        SuperBloque sb;
        TablaInodo ti;
        BloqueCarpeta bc;
        Journaling journal;

        sb.s_mtime = time(nullptr);
        sb.s_umtime = 0;
        sb.s_mnt_count = 1;
        sb.s_magic = 0xEF53;

        //Formatear particion Primaria
        if (nodo->part_type == 'P') {
            int p = -1;
            for (int i = 0; i < 4; i++) {
                if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                    mbr.mbr_partition_[i].part_status != '0' && mbr.mbr_partition_[i].part_type == 'P') {
                    p = i;
                    break;
                }
            }

            if (p != -1 && p != 4) {
                //Llenar de \0
                if (type == 2) {
                    fseek(archivo, mbr.mbr_partition_[p].part_start, SEEK_SET);
                    char vacio = '\0';
                    for (int i = 0; i < mbr.mbr_partition_[p].part_s; i++) { fwrite(&vacio, sizeof(char), 1, archivo); }
                }

                //Calcular el valor de N y Numero de Estructuras
                double n;
                if (sistemaArchivo == 1) {
                    n = (mbr.mbr_partition_[p].part_s - sizeof(SuperBloque)) / (4 + (sizeof(TablaInodo)) + (3 * sizeof(BloqueArchivo)));
                    sb.s_filesystem_type = 2;
                } else {
                    n = (mbr.mbr_partition_[p].part_s - sizeof(SuperBloque)) /
                        (4 + (sizeof(Journaling)) + (sizeof(TablaInodo)) + (3 * sizeof(BloqueArchivo)));
                    sb.s_filesystem_type = 3;
                }

                int num_estructura = floor(n);
                int bloques = 3 * num_estructura;

                //Inicializar SuperBloque
                sb.s_inodes_count = num_estructura;
                sb.s_blocks_count = bloques;
                sb.s_free_inodes_count = num_estructura - 2;
                sb.s_free_blocks_count = bloques - 2;
                sb.s_inode_size = sizeof(TablaInodo);
                sb.s_block_size = sizeof(BloqueArchivo);

                if (sistemaArchivo == 1) { sb.s_bm_inode_start = mbr.mbr_partition_[p].part_start + sizeof(SuperBloque); }
                else {
                    sb.s_bm_inode_start = mbr.mbr_partition_[p].part_start + sizeof(SuperBloque) + (num_estructura * sizeof(Journaling));
                }

                sb.s_first_ino = 2;
                sb.s_first_blo = 2;
                sb.s_bm_block_start = sb.s_bm_inode_start + num_estructura;
                sb.s_inode_start = sb.s_bm_block_start + bloques;
                sb.s_block_start = sb.s_inode_start + (num_estructura * sizeof(TablaInodo));

                //Inicializar Inodo Raiz
                ti.i_uid = 1;
                ti.i_gid = 1;
                ti.i_atime = time(nullptr);
                ti.i_ctime = time(nullptr);
                ti.i_mtime = time(nullptr);
                ti.i_perm = 664;
                ti.i_block[0] = sb.s_block_start;
                for (int i = 1; i < 15; i++) { ti.i_block[i] = -1; }
                ti.i_type = '0';
                ti.i_s = 0;

                fseek(archivo, sb.s_inode_start, SEEK_SET);
                fwrite(&ti, sizeof(TablaInodo), 1, archivo);

                strcpy(bc.b_content[0].b_name, ".");
                bc.b_content[0].b_inodo = sb.s_inode_start;
                strcpy(bc.b_content[1].b_name, "..");
                bc.b_content[1].b_inodo = sb.s_inode_start;
                strcpy(bc.b_content[2].b_name, "users.txt");
                bc.b_content[2].b_inodo = sb.s_inode_start + sizeof(TablaInodo);
                strcpy(bc.b_content[3].b_name, "");
                bc.b_content[3].b_inodo = -1;
                fseek(archivo, sb.s_block_start, SEEK_SET);
                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                //Generar Inodo y archivo Users.txt
                TablaInodo tiUsers;
                BloqueArchivo baUsers;

                tiUsers.i_uid = 1;
                tiUsers.i_gid = 1;
                tiUsers.i_atime = time(nullptr);
                tiUsers.i_ctime = time(nullptr);
                tiUsers.i_mtime = time(nullptr);
                tiUsers.i_perm = 700;
                tiUsers.i_block[0] = sb.s_block_start + sizeof(BloqueCarpeta);
                for (int i = 1; i < 15; i++) { tiUsers.i_block[i] = -1; }
                string s = "1,G,root\n1,U,root,root,123\n";
                tiUsers.i_s = s.size();
                tiUsers.i_type = '1';

                fseek(archivo, sb.s_inode_start + sizeof(TablaInodo), SEEK_SET);
                fwrite(&tiUsers, sizeof(TablaInodo), 1, archivo);

                memset(baUsers.b_content, '\0', sizeof(baUsers.b_content));
                strcpy(baUsers.b_content, s.c_str());
                fseek(archivo, sb.s_block_start + sizeof(BloqueCarpeta), SEEK_SET);
                fwrite(&baUsers, sizeof(BloqueArchivo), 1, archivo);

                //Inicializacion de Journaling para EXT3
                if (sistemaArchivo == 2) {
                    journal.sig = -1;
                    journal.tipo = '0';
                    journal.fecha = time(nullptr);
                    strcpy(journal.operacion, "mkfs");
                    strcpy(journal.path, "");
                    journal.start = mbr.mbr_partition_[p].part_start + sizeof(SuperBloque);
                }

                mbr.mbr_partition_[p].part_status = '2';

                //Actualizacion del MBR y escritura del superbloque en disco
                fseek(archivo, 0, SEEK_SET);
                fwrite(&mbr, sizeof(MBR), 1, archivo);
                fseek(archivo, mbr.mbr_partition_[p].part_start, SEEK_SET);
                fwrite(&sb, sizeof(SuperBloque), 1, archivo);

                //Escritura de los BITMAP
                char ch0 = '0';
                char ch1 = '1';
                fseek(archivo, sb.s_bm_inode_start, SEEK_SET);
                for (int i = 0; i < num_estructura; i++) { fwrite(&ch0, sizeof(char), 1, archivo); }
                fseek(archivo, sb.s_bm_inode_start, SEEK_SET);
                fwrite(&ch1, sizeof(char), 1, archivo);
                fwrite(&ch1, sizeof(char), 1, archivo);

                fseek(archivo, sb.s_bm_block_start, SEEK_SET);
                for (int i = 0; i < bloques; i++) { fwrite(&ch0, sizeof(char), 1, archivo); }
                fseek(archivo, sb.s_bm_block_start, SEEK_SET);
                fwrite(&ch1, sizeof(char), 1, archivo);
                fwrite(&ch1, sizeof(char), 1, archivo);

                if (sistemaArchivo == 2) {
                    fseek(archivo, mbr.mbr_partition_[p].part_start + sizeof(SuperBloque), SEEK_SET);
                    fwrite(&journal, sizeof(Journaling), 1, archivo);
                }

                string formato = "EXT2";
                if (sistemaArchivo == 2) {
                    formato = "EXT3";
                }
                cout << "La particion se formateo exitosamente " << formato << endl << endl;
            } else {
                this->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar la Particion en el Disco... " << endl;
                cout << "Se demontara la Particion " << endl << endl;
            }
        }
            //Formatear Particion Logica
        else if (nodo->part_type == 'E') {
            EBR ebr;
            fseek(archivo, nodo->part_start, SEEK_SET);
            fread(&ebr, sizeof(EBR), 1, archivo);

            if (strncmp(nodo->nombre_particion.c_str(), ebr.part_name, 16) == 0 && ebr.part_status != '0') {
                //Llenar de \0
                if (type == 2) {
                    fseek(archivo, ebr.part_start + sizeof(EBR), SEEK_SET);
                    char vacio = '\0';
                    for (int i = 0; i < ebr.part_s - sizeof(EBR); i++) { fwrite(&vacio, sizeof(char), 1, archivo); }
                }

                //Calcular el valor de N y Numero de Estructuras
                double n;
                if (sistemaArchivo == 1) {
                    n = ((ebr.part_s - (sizeof(EBR))) - sizeof(SuperBloque)) /
                        (4 + (sizeof(TablaInodo)) + (3 * sizeof(BloqueArchivo)));
                    sb.s_filesystem_type = 2;
                } else {
                    n = ((ebr.part_s - (sizeof(EBR))) - sizeof(SuperBloque)) /
                        (4 + (sizeof(Journaling)) + (sizeof(TablaInodo)) + (3 * sizeof(BloqueArchivo)));
                    sb.s_filesystem_type = 3;
                }

                int num_estructura = floor(n);
                int bloques = 3 * num_estructura;

                //Inicializar SuperBloque
                sb.s_inodes_count = num_estructura;
                sb.s_blocks_count = bloques;
                sb.s_free_inodes_count = num_estructura - 2;
                sb.s_free_blocks_count = bloques - 2;
                sb.s_inode_size = sizeof(TablaInodo);
                sb.s_block_size = sizeof(BloqueArchivo);

                if (sistemaArchivo == 1) { sb.s_bm_inode_start = (ebr.part_s + (sizeof(EBR))) + sizeof(SuperBloque); }
                else {
                    sb.s_bm_inode_start = (ebr.part_s + (sizeof(EBR))) + sizeof(SuperBloque) +
                                          (num_estructura * sizeof(Journaling));
                }

                sb.s_first_ino = 2;
                sb.s_first_blo = 2;
                sb.s_bm_block_start = sb.s_bm_inode_start + num_estructura;
                sb.s_inode_start = sb.s_bm_block_start + bloques;
                sb.s_block_start = sb.s_inode_start + (num_estructura * sizeof(TablaInodo));

                //Inicializar Inodo Raiz
                ti.i_uid = 1;
                ti.i_gid = 1;
                ti.i_atime = time(nullptr);
                ti.i_ctime = time(nullptr);
                ti.i_mtime = time(nullptr);
                ti.i_perm = 664;
                ti.i_block[0] = sb.s_block_start;
                for (int i = 1; i < 15; i++) { ti.i_block[i] = -1; }
                ti.i_type = '0';
                ti.i_s = 0;

                fseek(archivo, sb.s_inode_start, SEEK_SET);
                fwrite(&ti, sizeof(TablaInodo), 1, archivo);

                strcpy(bc.b_content[0].b_name, ".");
                bc.b_content[0].b_inodo = sb.s_inode_start;
                strcpy(bc.b_content[1].b_name, "..");
                bc.b_content[1].b_inodo = sb.s_inode_start;
                strcpy(bc.b_content[2].b_name, "users.txt");
                bc.b_content[2].b_inodo = sb.s_inode_start + sizeof(TablaInodo);
                strcpy(bc.b_content[3].b_name, "");
                bc.b_content[3].b_inodo = -1;
                fseek(archivo, sb.s_block_start, SEEK_SET);
                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                //Generar Inodo y archivo Users.txt
                TablaInodo tiUsers;
                BloqueArchivo baUsers;

                tiUsers.i_uid = 1;
                tiUsers.i_gid = 1;
                tiUsers.i_atime = time(nullptr);
                tiUsers.i_ctime = time(nullptr);
                tiUsers.i_mtime = time(nullptr);
                tiUsers.i_perm = 700;
                tiUsers.i_block[0] = sb.s_block_start + sizeof(BloqueCarpeta);
                for (int i = 1; i < 15; i++) { tiUsers.i_block[i] = -1; }
                string s = "1,G,root\n1,U,root,root,123\n";
                tiUsers.i_s = s.size();
                tiUsers.i_type = '1';

                fseek(archivo, sb.s_inode_start + sizeof(TablaInodo), SEEK_SET);
                fwrite(&tiUsers, sizeof(TablaInodo), 1, archivo);

                memset(baUsers.b_content, '\0', sizeof(baUsers.b_content));
                strcpy(baUsers.b_content, s.c_str());
                fseek(archivo, sb.s_block_start + sizeof(BloqueCarpeta), SEEK_SET);
                fwrite(&baUsers, sizeof(BloqueArchivo), 1, archivo);

                //Inicializacion de Journaling para EXT3
                if (sistemaArchivo == 2) {
                    journal.sig = -1;
                    journal.tipo = '0';
                    journal.fecha = time(nullptr);
                    strcpy(journal.operacion, "mkfs");
                    strcpy(journal.path, "");
                    journal.start = (ebr.part_s + (sizeof(EBR))) + sizeof(SuperBloque);
                }

                ebr.part_status = '2';

                //Escritura en disco del ebr y el superbloque
                fseek(archivo, ebr.part_start, SEEK_SET);
                fwrite(&ebr, sizeof(EBR), 1, archivo);
                fseek(archivo, ebr.part_start + sizeof(EBR), SEEK_SET);
                fwrite(&sb, sizeof(SuperBloque), 1, archivo);

                //Escrirtura de los BITMAP
                char ch0 = '0';
                char ch1 = '1';
                fseek(archivo, sb.s_bm_inode_start, SEEK_SET);
                for (int i = 0; i < num_estructura; i++) { fwrite(&ch0, sizeof(char), 1, archivo); }
                fseek(archivo, sb.s_bm_inode_start, SEEK_SET);
                fwrite(&ch1, sizeof(char), 1, archivo);
                fwrite(&ch1, sizeof(char), 1, archivo);

                fseek(archivo, sb.s_bm_block_start, SEEK_SET);
                for (int i = 0; i < bloques; i++) { fwrite(&ch0, sizeof(char), 1, archivo); }
                fseek(archivo, sb.s_bm_block_start, SEEK_SET);
                fwrite(&ch1, sizeof(char), 1, archivo);
                fwrite(&ch1, sizeof(char), 1, archivo);

                if (sistemaArchivo == 2) {
                    fseek(archivo, ebr.part_start + sizeof(EBR) + sizeof(SuperBloque), SEEK_SET);
                    fwrite(&journal, sizeof(Journaling), 1, archivo);
                }

                string formato = "EXT2";
                if (sistemaArchivo == 2) {
                    formato = "EXT3";
                }
                cout << "La particion se formateo exitosamente " << formato << endl << endl;
            } else {
                this->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar la Particion en el Disco... " << endl;
                cout << "Se demontara la Particion " << endl << endl;
            }
        }
        fclose(archivo);
    } else{
        this->eliminar(nodo->idCompleto);
        cout << "No fue posible encontrar el Disco... " << endl;
        cout << "Se demontara la Particion " << endl << endl;
    }
}

//Comando Mkfs
void ListaMount::mkfs(std::string id, std::string type, std::string fs) {
    int valorType = this->obtenerType(type);
    int valorFs = this->obtenerFs(fs);
    NodoMount * nodo = this->buscar(id);

    if(valorType != -1 && valorFs != -1 && nodo != NULL){
        this->ext(valorType, valorFs, nodo);
    }
}