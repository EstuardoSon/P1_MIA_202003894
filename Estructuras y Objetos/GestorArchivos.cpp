//
// Created by estuardo on 26/02/23.
//

#include <regex>
#include "GestorArchivos.h"

GestorArchivos::GestorArchivos(ListaMount *listaMount, Usuario *usuario) {
    this->listaMount = listaMount;
    this->usuario = usuario;
}

void GestorArchivos::chmod(string path, int ugo, bool r) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    else if(!this->verificarUGO(ugo)){
        cout << "Se ingreso un valor invalido en UGO" << endl << endl;
        return;
    }
    else{
        NodoMount * nodo = this->listaMount->buscar(this->usuario->idParticion);
        if (nodo != NULL) {
            FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
            if (archivo != NULL) {
                SuperBloque sb;
                int inicioSB = 0;

                //Particion Primaria
                if (nodo->part_type == 'P') {
                    MBR mbr;
                    fseek(archivo, 0, SEEK_SET);
                    fread(&mbr, sizeof(MBR), 1, archivo);
                    int i;

                    //Verificar la existencia de la particion
                    for (i = 0; i < 4; i++) {
                        if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            inicioSB = mbr.mbr_partition_[i].part_start;
                            break;
                        }
                    }

                    //Error de posicion no encontrada
                    if (i == 5) {
                        listaMount->eliminar(nodo->idCompleto);
                        cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                        fclose(archivo);
                        return;
                    }

                        //Posicion si Encontrada
                    else {
                        if (mbr.mbr_partition_[i].part_status != '2') {
                            cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                            fclose(archivo);
                            return;
                        }
                        //Recuperar la informacion del superbloque
                        fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                        fread(&sb, sizeof(SuperBloque), 1, archivo);
                    }
                }

                    //Particiones Logicas
                else if (nodo->part_type == 'L') {
                    EBR ebr;
                    fseek(archivo, nodo->part_start, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                    if (ebr.part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    inicioSB = nodo->part_start;
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }

                regex expresion(
                        "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
                if (!regex_match(path, expresion)) {
                    cout << "Ruta no valida " << endl << endl;
                }
                else{
                    path = path.erase(0, 1);
                    vector<string> ficheros = this->split(path, '/');

                    int ubicacion = this->buscarficheroChmod(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                    if(ubicacion != -1){
                        TablaInodo ti;
                        fseek(archivo, ubicacion, SEEK_SET);
                        fread(&ti, sizeof(TablaInodo), 1, archivo);

                        if(ti.i_type == '1' && ((ti.i_uid == this->usuario->idU && ti.i_gid == this->usuario->idG) || (this->usuario->nombreG == "root" && this->usuario->nombreU == "root"))){
                            ti.i_perm = ugo;
                            ti.i_mtime = time(nullptr);

                            cout << "Cambio Realizado" << endl << endl;
                        }
                        else if (ti.i_type == '0' && ((ti.i_uid == this->usuario->idU && ti.i_gid == this->usuario->idG) || (this->usuario->nombreG == "root" && this->usuario->nombreU == "root"))){
                            if(r){
                                BloqueApuntadores bap, bap1, bap2;
                                BloqueCarpeta bc;
                                for (int i = 0; i < 15; i++) {
                                    if (ti.i_block[i] != -1) {
                                        if (i == 0) {
                                            fseek(archivo, ti.i_block[i], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for(int c = 2; c < 4; c++){
                                                if(bc.b_content[c].b_inodo != -1){ this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);  }
                                            }
                                        }
                                        else if (i < 12) {
                                            fseek(archivo, ti.i_block[i], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for(int c = 0; c < 4; c++){
                                                if(bc.b_content[c].b_inodo != -1){ this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);  }
                                            }
                                        }
                                        else if (i == 12) {
                                            fseek(archivo, ti.i_block[i], SEEK_SET);
                                            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                            for(int j = 0; j < 16; j++){
                                                if (bap.b_pointers[j] != -1) {
                                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                    for (int c = 0; c < 4; c++) {
                                                        if (bc.b_content[c].b_inodo != -1) {
                                                            this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else if (i == 13) {
                                            fseek(archivo, ti.i_block[i], SEEK_SET);
                                            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                            for(int j = 0; j < 16; j++){
                                                if (bap.b_pointers[j] != -1) {
                                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                                    for(int k = 0; k < 16; k++){
                                                        if (bap1.b_pointers[k] != -1) {
                                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                            for (int c = 0; c < 4; c++) {
                                                                if (bc.b_content[c].b_inodo != -1) {
                                                                    this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else if (i == 14) {
                                            fseek(archivo, ti.i_block[i], SEEK_SET);
                                            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                            for(int j = 0; j < 16; j++){
                                                if (bap.b_pointers[j] != -1) {
                                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                                    for(int k = 0; k < 16; k++){
                                                        if (bap1.b_pointers[k] != -1) {
                                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                            fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                                            for(int l = 0; l < 16; l++){
                                                                if (bap2.b_pointers[l] != -1) {
                                                                    fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                    for (int c = 0; c < 4; c++) {
                                                                        if (bc.b_content[c].b_inodo != -1) {
                                                                            this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            ti.i_perm = ugo;
                            ti.i_mtime = time(nullptr);

                            cout << "Cambio Realizado" << endl << endl;
                        }
                        else{ cout << "El usuario no es el propietario del archivo o carpeta" << endl << endl; }

                        fseek(archivo, ubicacion, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    }
                }

                fclose(archivo);
            } else {
                listaMount->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar el disco de la particion" << endl << endl;
                return;
            }
        }
    }
}

//Realizar acciones del comando Chmod
int GestorArchivos::buscarficheroChmod(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    if (ficheros.size() > 0) {
        if (ti.i_type == '0') {
            string fichero = ficheros[0];
            ficheros.erase(ficheros.begin());
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, fichero);

            if (ubicacion != -1) {
                return this->buscarficheroChmod(ficheros,sb,inicioSB,ubicacion,archivo);

            } else { cout << "No se encontro el fichero " << fichero << endl << endl; return -1; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; return -1; }
    }
    else {
        return inicioInodo;
    }
}

//Buscar una carpeta o archivo en una carpeta padre
void GestorArchivos::buscarEnCarpetaChmod(int ubicacion, int ugo, FILE *archivo) {
    TablaInodo ti;
    fseek(archivo, ubicacion, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    if(ti.i_type == '1' && ((ti.i_uid == this->usuario->idU && ti.i_gid == this->usuario->idG) || (this->usuario->nombreG == "root" && this->usuario->nombreU == "root"))){
        ti.i_perm = ugo;
        ti.i_mtime = time(nullptr);
    }
    else if (ti.i_type == '0' && ((ti.i_uid == this->usuario->idU && ti.i_gid == this->usuario->idG) || (this->usuario->nombreG == "root" && this->usuario->nombreU == "root"))) {
        BloqueApuntadores bap, bap1, bap2;
        BloqueCarpeta bc;
        for (int i = 0; i < 15; i++) {
            if (ti.i_block[i] != -1) {
                if (i == 0) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for(int c = 2; c < 4; c++){
                        if(bc.b_content[c].b_inodo != -1){ this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);  }
                    }
                }
                else if (i < 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for(int c = 0; c < 4; c++){
                        if(bc.b_content[c].b_inodo != -1){ this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);  }
                    }
                }
                else if (i == 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                            for (int c = 0; c < 4; c++) {
                                if (bc.b_content[c].b_inodo != -1) {
                                    this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                }
                            }
                        }
                    }
                }
                else if (i == 13) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for(int k = 0; k < 16; k++){
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 0; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1) {
                                            this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else if (i == 14) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for(int k = 0; k < 16; k++){
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                    for(int l = 0; l < 16; l++){
                                        if (bap2.b_pointers[l] != -1) {
                                            fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int c = 0; c < 4; c++) {
                                                if (bc.b_content[c].b_inodo != -1) {
                                                    this->buscarEnCarpetaChmod(bc.b_content[c].b_inodo, ugo, archivo);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        ti.i_perm = ugo;
        ti.i_mtime = time(nullptr);
    }

    fseek(archivo, ubicacion, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
}


void GestorArchivos::mkfile(string path_fichero, string path_archivo, bool r, int size, string cont_fichero, string cont_archivo) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount * nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            if (this->verificarPath(path_fichero) && this->verificarArchivo(path_archivo)) {
                path_fichero = path_fichero.erase(0, 1);
                vector<string> ficheros = this->split(path_fichero,'/');

                //Texto a escribir
                bool bandera = true;
                string textoArchivo = "";
                if (cont_fichero != "" && cont_archivo != "") {
                    if (this->verificarPath(cont_fichero) && this->verificarArchivo(cont_archivo)) {
                        FILE *file = fopen((cont_fichero + "/" + cont_archivo).c_str(), "r");
                        if (file != NULL) {
                            char comando[1024];
                            while (fgets(comando, 1024, file)) {
                                string cadena = comando;
                                textoArchivo += cadena;
                            }
                            fclose(file);
                        } else{
                            cout << "No fue posible encontrar el archivo especificado en CONT" << endl;
                            bandera = false;
                        }
                    }
                }
                if(size >= 0){
                    for(int i = 0; i < size; i++){
                        int modulo = i % 10;
                        textoArchivo += to_string(modulo);
                    }
                }
                else{ bandera = false;  cout << "El valor de SIZE debe ser positivo" << endl; }

                if(bandera){
                    //Ejecutar
                    if(textoArchivo.size() <= 280320) {
                        this->buscarfichero(ficheros, path_archivo, r, sb, inicioSB, sb.s_inode_start, archivo, textoArchivo);
                    }
                    else{
                        cout << "El texto que desea ingresar excede el espacio disponible para un archivo" << endl;
                    }
                }
                else{ cout << endl; }
            }

            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

//Realizar acciones del comando mkfile
void GestorArchivos::buscarfichero(vector<string> &ficheros, string nombreArchivo, bool r, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo, string textoArchivo) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);
    bool escritura = false, lectura = false;
    this->verificarPermisos(ti, escritura, lectura);

    if (ficheros.size() > 0) {
        if (ti.i_type == '0') {
            string fichero = ficheros[0];
            ficheros.erase(ficheros.begin());
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, fichero);

            if (ubicacion != -1) {
                this->buscarfichero(ficheros, nombreArchivo, r, sb, inicioSB, ubicacion, archivo, textoArchivo);

            } else if (ubicacion == -1 && r) {
                if (escritura) {
                    ubicacion = this->buscarEspacioCarpeta(ti, inicioInodo, archivo, fichero, sb, inicioSB);
                    if (ubicacion != -1) {
                        cout << "Fichero Creado: " << fichero << endl;
                        this->buscarfichero(ficheros, nombreArchivo, r, sb, inicioSB, ubicacion, archivo, textoArchivo);

                        ti.i_mtime = time(nullptr);
                        fseek(archivo, inicioInodo, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    } else {
                        return;
                    }
                } else { cout << "El usuario no tiene permisos de Escritura" << endl << endl; }
            } else { cout << "No se encontro el fichero " << fichero << endl << endl; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
    }
    else {
        if (ti.i_type == '0') {
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, nombreArchivo);

            if (ubicacion != -1) {
                cout << "El archivo ya existe" << endl << endl;

            } else {
                if (escritura) {
                    ubicacion = this->buscarEspacioArchivo(ti, inicioInodo, archivo, nombreArchivo, sb, inicioSB,
                                                           textoArchivo);
                    if (ubicacion != -1) {
                        cout << "Archivo Creado: " << nombreArchivo << endl << endl;

                        ti.i_mtime = time(nullptr);
                        fseek(archivo, inicioInodo, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    } else {
                        cout << endl;
                        return;
                    }
                } else { cout << "El usuario no tiene permisos de Escritura" << endl << endl; }
            }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
    }
}


void GestorArchivos::cat(vector<string> rutas) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount * nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            int i;
            regex expresion(
                    "(\\/)([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)");
            for(i = 0; i < rutas.size(); i++){
                if (!regex_match(rutas[i], expresion)) {
                    cout << "Ruta no valida: " << rutas[i] << endl;
                }
                else{
                    vector<string> ficheros = this->split(rutas[i],'/');
                    string nombreArchivo = ficheros.back();
                    ficheros.pop_back();
                    this->buscarficheroCat(ficheros,nombreArchivo,sb,inicioSB,sb.s_inode_start,archivo);
                }
            }

            cout << endl;
            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

//Realizar acciones del comando cat
void GestorArchivos::buscarficheroCat(vector<string> &ficheros, string nombreArchivo, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);
    bool escritura = false, lectura = false;
    this->verificarPermisos(ti, escritura, lectura);

    if (ficheros.size() > 0) {
        if(lectura){
            if (ti.i_type == '0') {
                string fichero = ficheros[0];
                ficheros.erase(ficheros.begin());
                int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo,fichero);

                if (ubicacion != -1) {
                    this->buscarficheroCat(ficheros, nombreArchivo, sb, inicioSB, ubicacion, archivo);
                } else { cout << "No se encontro el fichero " << fichero << endl << endl; }
            }
            else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
        } else{ cout << "El usuario no tiene permisos de Lectura en: " << ficheros[0] << endl << endl; }
    }
    else{
        if(lectura){
            if (ti.i_type == '0') {
                int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo,nombreArchivo);

                if (ubicacion != -1) {
                    TablaInodo iArchivo;
                    fseek(archivo, ubicacion, SEEK_SET);
                    fread(&iArchivo, sizeof(TablaInodo), 1, archivo);

                    if(iArchivo.i_type == '1') {
                        escritura=false;
                        lectura=false;
                        this->verificarPermisos(iArchivo, escritura, lectura);
                        if(lectura) {
                            cout << this->getContentF(ubicacion, archivo) << endl << endl;
                        } else{ cout << "El usuario no tiene permisos de Lectura" << endl << endl; }
                    }else { cout << "El inodo no corresponde a un archivo" << endl << endl; }
                } else { cout << "El archivo no existe" << endl << endl; }
            }
            else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
        } else{ cout << "El usuario no tiene permisos de Lectura en el fichero padre" << endl << endl; }
    }
}


void GestorArchivos::remove(string path) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    else{
        NodoMount * nodo = this->listaMount->buscar(this->usuario->idParticion);
        if (nodo != NULL) {
            FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
            if (archivo != NULL) {
                SuperBloque sb;
                int inicioSB = 0;

                //Particion Primaria
                if (nodo->part_type == 'P') {
                    MBR mbr;
                    fseek(archivo, 0, SEEK_SET);
                    fread(&mbr, sizeof(MBR), 1, archivo);
                    int i;

                    //Verificar la existencia de la particion
                    for (i = 0; i < 4; i++) {
                        if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            inicioSB = mbr.mbr_partition_[i].part_start;
                            break;
                        }
                    }

                    //Error de posicion no encontrada
                    if (i == 5) {
                        listaMount->eliminar(nodo->idCompleto);
                        cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                        fclose(archivo);
                        return;
                    }

                        //Posicion si Encontrada
                    else {
                        if (mbr.mbr_partition_[i].part_status != '2') {
                            cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                            fclose(archivo);
                            return;
                        }
                        //Recuperar la informacion del superbloque
                        fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                        fread(&sb, sizeof(SuperBloque), 1, archivo);
                    }
                }

                    //Particiones Logicas
                else if (nodo->part_type == 'L') {
                    EBR ebr;
                    fseek(archivo, nodo->part_start, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                    if (ebr.part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    inicioSB = nodo->part_start;
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }

                regex expresion(
                        "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
                if (!regex_match(path, expresion)) {
                    cout << "Ruta no valida " << endl << endl;
                }
                else{
                    path = path.erase(0, 1);
                    vector<string> ficheros = this->split(path, '/');

                    int ubicacion = this->buscarficheroRemove(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                    if(ubicacion != -1){
                        TablaInodo ti;
                        fseek(archivo, ubicacion, SEEK_SET);
                        fread(&ti, sizeof(TablaInodo), 1, archivo);

                        if(ti.i_type == '1'){
                            cout << "El nombre no correponde a una carpeta" << endl << endl;
                        }
                        else if (ti.i_type == '0') {
                            BloqueApuntadores bap, bap1, bap2;
                            BloqueCarpeta bc;
                            TablaInodo tiAux;
                            char vacio = '\0', cero = '0';
                            for (int i = 0; i < 15; i++) {
                                if (ti.i_block[i] != -1) {
                                    if (i == 0) {
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                        for (int c = 2; c < 4; c++) {
                                            if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                bool escritura, lectura;
                                                this->verificarPermisos(tiAux,escritura,lectura);

                                                if(escritura) {
                                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                       inicioSB)) {
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/
                                                                        sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                        fwrite(&cero, sizeof(char), 1, archivo);

                                                        sb.s_free_inodes_count++;

                                                        strcpy(bc.b_content[c].b_name, "");
                                                        bc.b_content[c].b_inodo = -1;
                                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                        ti.i_mtime = time(nullptr);
                                                    }
                                                    cout << "Proceso Finalizado con Exito" << endl << endl;
                                                } else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                goto t0;
                                            }
                                        }
                                    }
                                    else if (i < 12) {
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                        for (int c = 0; c < 4; c++) {
                                            if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                bool escritura, lectura;
                                                this->verificarPermisos(tiAux,escritura,lectura);

                                                if(escritura) {
                                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                       inicioSB)) {
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                        fwrite(&cero, sizeof(char), 1, archivo);

                                                        sb.s_free_inodes_count++;

                                                        strcpy(bc.b_content[c].b_name, "");
                                                        bc.b_content[c].b_inodo = -1;
                                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                        ti.i_mtime = time(nullptr);
                                                    }
                                                    cout << "Proceso Finalizado con Exito" << endl << endl;
                                                }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                goto t0;
                                            }
                                        }
                                    }
                                    else if (i == 12) {
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                        for (int j = 0; j < 16; j++) {
                                            if (bap.b_pointers[j] != -1) {
                                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                for (int c = 0; c < 4; c++) {
                                                    if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                        bool escritura, lectura;
                                                        this->verificarPermisos(tiAux,escritura,lectura);

                                                        if(escritura) {
                                                            if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                               inicioSB)) {
                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                                fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                                fwrite(&cero, sizeof(char), 1, archivo);

                                                                sb.s_free_inodes_count++;

                                                                strcpy(bc.b_content[c].b_name, "");
                                                                bc.b_content[c].b_inodo = -1;
                                                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                ti.i_mtime = time(nullptr);
                                                            }
                                                            cout << "Proceso Finalizado con Exito" << endl << endl;
                                                        }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                        goto t0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else if (i == 13) {
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                        for (int j = 0; j < 16; j++) {
                                            if (bap.b_pointers[j] != -1) {
                                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                                for (int k = 0; k < 16; k++) {
                                                    if (bap1.b_pointers[k] != -1) {
                                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                        for (int c = 0; c < 4; c++) {
                                                            if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                bool escritura, lectura;
                                                                this->verificarPermisos(tiAux,escritura,lectura);

                                                                if(escritura) {
                                                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                                       inicioSB)) {
                                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                                        fwrite(&cero, sizeof(char), 1, archivo);

                                                                        sb.s_free_inodes_count++;

                                                                        strcpy(bc.b_content[c].b_name, "");
                                                                        bc.b_content[c].b_inodo = -1;
                                                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                        ti.i_mtime = time(nullptr);
                                                                    }
                                                                    cout << "Proceso Finalizado con Exito" << endl << endl;
                                                                }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                                goto t0;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else if (i == 14) {
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                        for (int j = 0; j < 16; j++) {
                                            if (bap.b_pointers[j] != -1) {
                                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                                for (int k = 0; k < 16; k++) {
                                                    if (bap1.b_pointers[k] != -1) {
                                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                        fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                                        for (int l = 0; l < 16; l++) {
                                                            if (bap2.b_pointers[l] != -1) {
                                                                fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                for (int c = 0; c < 4; c++) {
                                                                    if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                        fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                        bool escritura, lectura;
                                                                        this->verificarPermisos(tiAux,escritura,lectura);

                                                                        if(escritura) {
                                                                            if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                                               inicioSB)) {
                                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                                for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                                                fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                                                fwrite(&cero, sizeof(char), 1, archivo);

                                                                                sb.s_free_inodes_count++;

                                                                                strcpy(bc.b_content[c].b_name, "");
                                                                                bc.b_content[c].b_inodo = -1;
                                                                                fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                                ti.i_mtime = time(nullptr);

                                                                            }
                                                                            cout << "Proceso Finalizado con Exito" << endl << endl;
                                                                        }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                                        goto t0;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            cout << "Ruta no encontrada" << endl << endl;
                        }

                        t0:
                        fseek(archivo, ubicacion, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    }
                }

                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                sb.s_first_ino = this->buscarBM_i(sb, archivo);
                fseek(archivo, inicioSB, SEEK_SET);
                fwrite(&sb, sizeof(SuperBloque), 1, archivo);
                fclose(archivo);
            } else {
                listaMount->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar el disco de la particion" << endl << endl;
                return;
            }
        }
    }
}

//Realizar acciones del comando Remove
int GestorArchivos::buscarficheroRemove(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    if (ficheros.size() > 1) {
        if (ti.i_type == '0') {
            string fichero = ficheros[0];
            ficheros.erase(ficheros.begin());
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, fichero);

            if (ubicacion != -1) {
                return this->buscarficheroRemove(ficheros,sb,inicioSB,ubicacion,archivo);

            } else { cout << "No se encontro el fichero " << fichero << endl << endl; return -1; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; return -1; }
    }
    else {
        return inicioInodo;
    }
}

//Borrar Directorio o Archivo
bool GestorArchivos::eliminar(int inicioInodo, FILE *archivo, SuperBloque &sb, int inicioSB) {
    TablaInodo ti;
    BloqueApuntadores bap, bap1, bap2;
    char vacio = '\0', cero = '0';

    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    bool escritura, lectura;
    this->verificarPermisos(ti,escritura,lectura);

    if(escritura){
        if(ti.i_type == '1'){
            BloqueArchivo bA;
            for(int i = 0; i < 15; i++){
                if(ti.i_block[i] != -1){
                    if(i < 12){
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        for(int c = 0; c < 64; c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                        fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                        fwrite(&cero, sizeof(char), 1, archivo);
                        sb.s_free_blocks_count++;
                    }
                    else if(i == 12){
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for(int j = 0; j < 16; j++){
                            if(bap.b_pointers[j] != -1){
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                for(int c = 0; c < 64; c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                fseek(archivo, ((bap.b_pointers[j]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                                fwrite(&cero, sizeof(char), 1, archivo);
                                sb.s_free_blocks_count++;
                            }
                        }

                        fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                        fwrite(&cero, sizeof(char), 1, archivo);
                        sb.s_free_blocks_count++;
                    }
                    else if(i == 13){
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for(int j = 0; j < 16; j++){
                            if(bap.b_pointers[j] != -1){
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                for(int k = 0; k < 16; k++){
                                    if(bap1.b_pointers[k] != -1){
                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                        for(int c = 0; c < 64; c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                        fseek(archivo, ((bap1.b_pointers[k]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                                        fwrite(&cero, sizeof(char), 1, archivo);
                                        sb.s_free_blocks_count++;
                                    }
                                }

                                fseek(archivo, ((bap.b_pointers[j]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                                fwrite(&cero, sizeof(char), 1, archivo);
                                sb.s_free_blocks_count++;
                            }
                        }

                        fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/ sizeof(BloqueArchivo))+sb.s_bm_block_start, SEEK_SET);
                        fwrite(&cero, sizeof(char), 1, archivo);
                        sb.s_free_blocks_count++;
                    }
                    else if(i == 14) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for (int j = 0; j < 16; j++) {
                            if (bap.b_pointers[j] != -1) {
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                for (int k = 0; k < 16; k++) {
                                    if (bap1.b_pointers[k] != -1) {
                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                        fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                        for (int l = 0; l < 16; l++) {
                                            if (bap2.b_pointers[l] != -1) {
                                                fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                for (int c = 0; c < 64; c++) {
                                                    fwrite(&vacio, sizeof(char), 1, archivo);
                                                }
                                                fseek(archivo,
                                                      ((bap2.b_pointers[l] - sb.s_block_start) /
                                                       sizeof(BloqueArchivo)) +
                                                      sb.s_bm_block_start, SEEK_SET);
                                                fwrite(&cero, sizeof(char), 1, archivo);
                                                sb.s_free_blocks_count++;
                                            }
                                        }

                                        fseek(archivo, ((bap1.b_pointers[k] - sb.s_block_start) / sizeof(BloqueArchivo)) +
                                                       sb.s_bm_block_start, SEEK_SET);
                                        fwrite(&cero, sizeof(char), 1, archivo);
                                        sb.s_free_blocks_count++;
                                    }
                                }

                                fseek(archivo, ((bap.b_pointers[j] - sb.s_block_start) / sizeof(BloqueArchivo)) +
                                               sb.s_bm_block_start, SEEK_SET);
                                fwrite(&cero, sizeof(char), 1, archivo);
                                sb.s_free_blocks_count++;
                            }
                        }

                        fseek(archivo,
                              ((ti.i_block[i] - sb.s_block_start) / sizeof(BloqueArchivo)) + sb.s_bm_block_start,
                              SEEK_SET);
                        fwrite(&cero, sizeof(char), 1, archivo);
                        sb.s_free_blocks_count++;
                    }
                }

                ti.i_block[i] = -1;
            }
            return true;
        }
        else if (ti.i_type == '0') {
            BloqueApuntadores bap, bap1, bap2;
            BloqueCarpeta bc;
            TablaInodo tiAux;
            char vacio = '\0', cero = '0';
            for (int i = 0; i < 15; i++) {
                if (ti.i_block[i] != -1) {
                    if (i == 0) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                        for (int c = 2; c < 4; c++) {
                            if (bc.b_content[c].b_inodo != -1) {
                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                bool escritura, lectura;
                                this->verificarPermisos(tiAux,escritura,lectura);

                                if(escritura) {
                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                       inicioSB)) {
                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                        fwrite(&cero, sizeof(char), 1, archivo);

                                        sb.s_free_inodes_count++;

                                        strcpy(bc.b_content[c].b_name, "");
                                        bc.b_content[c].b_inodo = -1;
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                        ti.i_mtime = time(nullptr);
                                    }
                                }
                            }
                        }
                    }
                    else if (i < 12) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                        for (int c = 0; c < 4; c++) {
                            if (bc.b_content[c].b_inodo != -1) {
                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                bool escritura, lectura;
                                this->verificarPermisos(tiAux,escritura,lectura);

                                if(escritura) {
                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                       inicioSB)) {
                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                        fwrite(&cero, sizeof(char), 1, archivo);

                                        sb.s_free_inodes_count++;

                                        strcpy(bc.b_content[c].b_name, "");
                                        bc.b_content[c].b_inodo = -1;
                                        fseek(archivo, ti.i_block[i], SEEK_SET);
                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                        ti.i_mtime = time(nullptr);
                                    }
                                }
                            }
                        }

                        bool verificar = true;
                        for (int c = 0; c < 4; c++){
                            if (bc.b_content[c].b_inodo != -1){
                                verificar = false;
                                break;
                            }
                        }

                        if(verificar){
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                            fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                            fwrite(&cero, sizeof(char), 1, archivo);
                            ti.i_block[i] = -1;
                            sb.s_free_blocks_count++;
                            ti.i_mtime = time(nullptr);
                        }
                        else {
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        }
                    }
                    else if (i == 12) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for (int j = 0; j < 16; j++) {
                            if (bap.b_pointers[j] != -1) {
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                for (int c = 0; c < 4; c++) {
                                    if (bc.b_content[c].b_inodo != -1) {
                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                        fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                        bool escritura, lectura;
                                        this->verificarPermisos(tiAux,escritura,lectura);

                                        if(escritura) {
                                            if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                               inicioSB)) {
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                for (int e = 0; e < sizeof(TablaInodo); e++) {
                                                    fwrite(&vacio, sizeof(char), 1, archivo);
                                                }
                                                fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start) / sizeof(TablaInodo)) +
                                                               sb.s_bm_inode_start, SEEK_SET);
                                                fwrite(&cero, sizeof(char), 1, archivo);

                                                sb.s_free_inodes_count++;

                                                strcpy(bc.b_content[c].b_name, "");
                                                bc.b_content[c].b_inodo = -1;
                                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                ti.i_mtime = time(nullptr);
                                            }
                                        }
                                    }
                                }

                                bool verificar = true;
                                for (int c = 0; c < 4; c++){
                                    if (bc.b_content[c].b_inodo != -1){
                                        verificar = false;
                                        break;
                                    }
                                }

                                if(verificar){
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                    fseek(archivo, ((bap.b_pointers[j]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                                    fwrite(&cero, sizeof(char), 1, archivo);
                                    bap.b_pointers[j] = -1;
                                    sb.s_free_blocks_count++;
                                    ti.i_mtime = time(nullptr);
                                }
                                else {
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                }
                            }
                        }

                        bool verificar = true;
                        for (int c = 0; c < 16; c++){
                            if (bap.b_pointers[c] != -1){
                                verificar = false;
                                break;
                            }
                        }

                        if(verificar){
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                            fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                            fwrite(&cero, sizeof(char), 1, archivo);
                            ti.i_block[i] = -1;
                            sb.s_free_blocks_count++;
                            ti.i_mtime = time(nullptr);
                        }
                        else {
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            fwrite(&bap, sizeof(BloqueCarpeta), 1, archivo);
                        }
                    }
                    else if (i == 13) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for (int j = 0; j < 16; j++) {
                            if (bap.b_pointers[j] != -1) {
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                for (int k = 0; k < 16; k++) {
                                    if (bap1.b_pointers[k] != -1) {
                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                        for (int c = 0; c < 4; c++) {
                                            if (bc.b_content[c].b_inodo != -1) {
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                bool escritura, lectura;
                                                this->verificarPermisos(tiAux,escritura,lectura);

                                                if(escritura) {
                                                    if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                       inicioSB)) {
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        for(int e = 0 ; e < sizeof(TablaInodo); e++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                                        fseek(archivo, ((bc.b_content[c].b_inodo - sb.s_inode_start)/sizeof(TablaInodo))+sb.s_bm_inode_start, SEEK_SET);
                                                        fwrite(&cero, sizeof(char), 1, archivo);

                                                        sb.s_free_inodes_count++;

                                                        strcpy(bc.b_content[c].b_name, "");
                                                        bc.b_content[c].b_inodo = -1;
                                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                        ti.i_mtime = time(nullptr);
                                                    }
                                                }
                                            }
                                        }

                                        bool verificar = true;
                                        for (int c = 0; c < 4; c++){
                                            if (bap1.b_pointers[c] != -1){
                                                verificar = false;
                                                break;
                                            }
                                        }

                                        if(verificar){
                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                            for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                            fseek(archivo, ((bap1.b_pointers[k]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                                            fwrite(&cero, sizeof(char), 1, archivo);
                                            bap1.b_pointers[k] = -1;
                                            sb.s_free_blocks_count++;
                                            ti.i_mtime = time(nullptr);
                                        }
                                        else {
                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                        }
                                    }
                                }

                                bool verificar = true;
                                for (int c = 0; c < 16; c++){
                                    if (bap1.b_pointers[c] != -1){
                                        verificar = false;
                                        break;
                                    }
                                }

                                if(verificar){
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                    fseek(archivo, ((bap.b_pointers[j]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                                    fwrite(&cero, sizeof(char), 1, archivo);
                                    bap.b_pointers[j] = -1;
                                    sb.s_free_blocks_count++;
                                    ti.i_mtime = time(nullptr);
                                }
                                else {
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    fwrite(&bap1, sizeof(BloqueCarpeta), 1, archivo);
                                }
                            }
                        }

                        bool verificar = true;
                        for (int c = 0; c < 16; c++){
                            if (bap.b_pointers[c] != -1){
                                verificar = false;
                                break;
                            }
                        }

                        if(verificar){
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                            fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                            fwrite(&cero, sizeof(char), 1, archivo);
                            ti.i_block[i] = -1;
                            sb.s_free_blocks_count++;
                            ti.i_mtime = time(nullptr);
                        }
                        else {
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            fwrite(&bap, sizeof(BloqueCarpeta), 1, archivo);
                        }
                    }
                    else if (i == 14) {
                        fseek(archivo, ti.i_block[i], SEEK_SET);
                        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                        for (int j = 0; j < 16; j++) {
                            if (bap.b_pointers[j] != -1) {
                                fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                for (int k = 0; k < 16; k++) {
                                    if (bap1.b_pointers[k] != -1) {
                                        fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                        fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                        for (int l = 0; l < 16; l++) {
                                            if (bap2.b_pointers[l] != -1) {
                                                fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                for (int c = 0; c < 4; c++) {
                                                    if (bc.b_content[c].b_inodo != -1) {
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                        bool escritura, lectura;
                                                        this->verificarPermisos(tiAux, escritura, lectura);

                                                        if (escritura) {
                                                            if (this->eliminar(bc.b_content[c].b_inodo, archivo, sb,
                                                                               inicioSB)) {
                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                for (int e = 0; e < sizeof(TablaInodo); e++) {
                                                                    fwrite(&vacio, sizeof(char), 1, archivo);
                                                                }
                                                                fseek(archivo,
                                                                      ((bc.b_content[c].b_inodo - sb.s_inode_start) /
                                                                       sizeof(TablaInodo)) + sb.s_bm_inode_start, SEEK_SET);
                                                                fwrite(&cero, sizeof(char), 1, archivo);

                                                                sb.s_free_inodes_count++;

                                                                strcpy(bc.b_content[c].b_name, "");
                                                                bc.b_content[c].b_inodo = -1;
                                                                fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                ti.i_mtime = time(nullptr);
                                                            }
                                                        }
                                                    }
                                                }

                                                bool verificar = true;
                                                for (int c = 0; c < 4; c++) {
                                                    if (bc.b_content[c].b_inodo != -1) {
                                                        verificar = false;
                                                        break;
                                                    }
                                                }

                                                if (verificar) {
                                                    fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                    for (int c = 0; c < sizeof(BloqueCarpeta); c++) {
                                                        fwrite(&vacio, sizeof(char), 1, archivo);
                                                    }
                                                    fseek(archivo, ((bap2.b_pointers[l] - sb.s_block_start) / 64) +
                                                                   sb.s_bm_block_start, SEEK_SET);
                                                    fwrite(&cero, sizeof(char), 1, archivo);
                                                    bap2.b_pointers[l] = -1;
                                                    sb.s_free_blocks_count++;
                                                    ti.i_mtime = time(nullptr);
                                                }
                                                else {
                                                    fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                }
                                            }
                                        }

                                        bool verificar = true;
                                        for (int c = 0; c < 16; c++) {
                                            if (bap2.b_pointers[c] != -1) {
                                                verificar = false;
                                                break;
                                            }
                                        }

                                        if (verificar) {
                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                            for (int c = 0; c < sizeof(BloqueCarpeta); c++) {
                                                fwrite(&vacio, sizeof(char), 1, archivo);
                                            }
                                            fseek(archivo,
                                                  ((bap1.b_pointers[k] - sb.s_block_start) / 64) + sb.s_bm_block_start,
                                                  SEEK_SET);
                                            fwrite(&cero, sizeof(char), 1, archivo);
                                            bap1.b_pointers[k] = -1;
                                            sb.s_free_blocks_count++;
                                        }
                                        else {
                                            fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                            fwrite(&bap2, sizeof(BloqueCarpeta), 1, archivo);
                                        }
                                    }
                                }

                                bool verificar = true;
                                for (int c = 0; c < 16; c++){
                                    if (bap1.b_pointers[c] != -1){
                                        verificar = false;
                                        break;
                                    }
                                }

                                if(verificar){
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                                    fseek(archivo, ((bap.b_pointers[j]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                                    fwrite(&cero, sizeof(char), 1, archivo);
                                    bap.b_pointers[j] = -1;
                                    sb.s_free_blocks_count++;
                                    ti.i_mtime = time(nullptr);
                                }
                                else {
                                    fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                    fwrite(&bap1, sizeof(BloqueCarpeta), 1, archivo);
                                }
                            }
                        }

                        bool verificar = true;
                        for (int c = 0; c < 16; c++){
                            if (bap.b_pointers[c] != -1){
                                verificar = false;
                                break;
                            }
                        }

                        if(verificar){
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            for(int c = 0; c < sizeof(BloqueCarpeta); c++){ fwrite(&vacio, sizeof(char), 1, archivo); }
                            fseek(archivo, ((ti.i_block[i]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                            fwrite(&cero, sizeof(char), 1, archivo);
                            ti.i_block[i] = -1;
                            sb.s_free_blocks_count++;
                            ti.i_mtime = time(nullptr);
                        }
                        else {
                            fseek(archivo, ti.i_block[i], SEEK_SET);
                            fwrite(&bap, sizeof(BloqueCarpeta), 1, archivo);
                        }
                    }
                }
            }

            fseek(archivo, inicioInodo, SEEK_SET);
            fwrite(&ti, sizeof(TablaInodo), 1, archivo);

            for (int c = 14; c >= 0; c--){
                if(c >= 1 && ti.i_block[c] != -1) {
                    fseek(archivo, inicioInodo, SEEK_SET);
                    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    return false;
                }
                else if(c == 0){
                    fseek(archivo, ti.i_block[c], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for(int n = 2; n < 4; n++){
                        if(bc.b_content[n].b_inodo != -1){
                            fseek(archivo, inicioInodo, SEEK_SET);
                            fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                            return false;
                        }
                    }

                    fseek(archivo, ti.i_block[c], SEEK_SET);
                    for (int e = 0; e < sizeof(BloqueCarpeta); e++) { fwrite(&vacio, sizeof(char), 1, archivo); }
                    fseek(archivo, ((ti.i_block[c]-sb.s_block_start)/64)+sb.s_bm_block_start, SEEK_SET);
                    fwrite(&cero, sizeof(char), 1, archivo);
                    sb.s_free_blocks_count++;
                    return true;
                }
            }
        }
    }
    else { return false; }
}


void GestorArchivos::edit(string path, string contenido) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    FILE * fileContent = fopen(contenido.c_str(), "r");
    if(fileContent != NULL) {
        NodoMount *nodo = this->listaMount->buscar(this->usuario->idParticion);
        if (nodo != NULL) {
            FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
            if (archivo != NULL) {
                SuperBloque sb;
                int inicioSB = 0;

                //Particion Primaria
                if (nodo->part_type == 'P') {
                    MBR mbr;
                    fseek(archivo, 0, SEEK_SET);
                    fread(&mbr, sizeof(MBR), 1, archivo);
                    int i;

                    //Verificar la existencia de la particion
                    for (i = 0; i < 4; i++) {
                        if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            inicioSB = mbr.mbr_partition_[i].part_start;
                            break;
                        }
                    }

                    //Error de posicion no encontrada
                    if (i == 5) {
                        listaMount->eliminar(nodo->idCompleto);
                        cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                        fclose(archivo);
                        return;
                    }

                        //Posicion si Encontrada
                    else {
                        if (mbr.mbr_partition_[i].part_status != '2') {
                            cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                            fclose(archivo);
                            return;
                        }
                        //Recuperar la informacion del superbloque
                        fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                        fread(&sb, sizeof(SuperBloque), 1, archivo);
                    }
                }

                    //Particiones Logicas
                else if (nodo->part_type == 'L') {
                    EBR ebr;
                    fseek(archivo, nodo->part_start, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                    if (ebr.part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    inicioSB = nodo->part_start;
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }

                regex expresion(
                        "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
                if (!regex_match(path, expresion)) {
                    cout << "Ruta no valida " << endl << endl;
                }
                else{
                    path = path.erase(0, 1);
                    vector<string> ficheros = this->split(path, '/');

                    int ubicacion = this->buscarficheroChmod(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                    if(ubicacion != -1){
                        TablaInodo ti;
                        fseek(archivo, ubicacion, SEEK_SET);
                        fread(&ti, sizeof(TablaInodo), 1, archivo);
                        string textoArchivo = "";

                        if(ti.i_type == '1'){
                            char comando[1024];
                            while (fgets(comando, 1024, fileContent)) {
                                string cadena = comando;
                                textoArchivo += cadena;
                            }

                            this->writeInFile(textoArchivo, sb, inicioSB, ubicacion, archivo);
                        }
                        else if (ti.i_type == '0') {
                            cout << "La ruta no corresponde a un archivo" << endl << endl;
                        }
                    }
                }

                fclose(archivo);
            } else {
                listaMount->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar el disco de la particion" << endl << endl;
                return;
            }
        }

        fclose(fileContent);
    } else { cout << "No fue posible encontrar el archivo requerido para el contenido" << endl << endl; }
}


void GestorArchivos::rename(string path, string name) {
    if (this->usuario->nombreG == "" && this->usuario->nombreU == "") {
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    if(name.size() >= 13 || name.size() <= 0){
        cout << "El nombre debe tener un maximo de 12 caracteres" << endl << endl;
        return;
    }
    NodoMount *nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            regex expresion(
                    "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
            if (!regex_match(path, expresion)) {
                cout << "Ruta no valida " << endl << endl;
            } else {
                path = path.erase(0, 1);
                vector<string> ficheros = this->split(path, '/');

                int ubicacion = this->buscarficheroRemove(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                if(ubicacion != -1){
                    TablaInodo ti;
                    fseek(archivo, ubicacion, SEEK_SET);
                    fread(&ti, sizeof(TablaInodo), 1, archivo);

                    if(ti.i_type == '1'){
                        cout << "El nombre no correponde a una carpeta" << endl << endl;
                    }
                    else if (ti.i_type == '0') {
                        BloqueApuntadores bap, bap1, bap2;
                        BloqueCarpeta bc;
                        TablaInodo tiAux;
                        char vacio = '\0', cero = '0';
                        for (int i = 0; i < 15; i++) {
                            if (ti.i_block[i] != -1) {
                                if (i == 0) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 2; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                            bool escritura, lectura;
                                            this->verificarPermisos(tiAux,escritura,lectura);

                                            if(escritura) {
                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                strcpy(bc.b_content[c].b_name,name.c_str());
                                                tiAux.i_mtime = time(nullptr);
                                                fseek(archivo, ti.i_block[i], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                            } else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                            goto  t0;
                                        }
                                    }
                                }
                                else if (i < 12) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 0; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                            bool escritura, lectura;
                                            this->verificarPermisos(tiAux,escritura,lectura);

                                            if(escritura) {
                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                strcpy(bc.b_content[c].b_name,name.c_str());
                                                tiAux.i_mtime = time(nullptr);
                                                fseek(archivo, ti.i_block[i], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                            }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                            goto t0;
                                        }
                                    }
                                }
                                else if (i == 12) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int c = 0; c < 4; c++) {
                                                if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                    fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                    fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                    bool escritura, lectura;
                                                    this->verificarPermisos(tiAux,escritura,lectura);

                                                    if(escritura) {
                                                        for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                        strcpy(bc.b_content[c].b_name,name.c_str());
                                                        tiAux.i_mtime = time(nullptr);
                                                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                        cout << "Proceso Finalizado con Exito" << endl << endl;
                                                    }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                    goto t0;
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (i == 13) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                            for (int k = 0; k < 16; k++) {
                                                if (bap1.b_pointers[k] != -1) {
                                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                    for (int c = 0; c < 4; c++) {
                                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                            bool escritura, lectura;
                                                            this->verificarPermisos(tiAux,escritura,lectura);

                                                            if(escritura) {
                                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                                strcpy(bc.b_content[c].b_name,name.c_str());
                                                                tiAux.i_mtime = time(nullptr);
                                                                fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                                            }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                            goto t0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (i == 14) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                            for (int k = 0; k < 16; k++) {
                                                if (bap1.b_pointers[k] != -1) {
                                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                    fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                                    for (int l = 0; l < 16; l++) {
                                                        if (bap2.b_pointers[l] != -1) {
                                                            fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                            for (int c = 0; c < 4; c++) {
                                                                if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                                    fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                    fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                    bool escritura, lectura;
                                                                    this->verificarPermisos(tiAux,escritura,lectura);

                                                                    if(escritura) {
                                                                        for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                                        strcpy(bc.b_content[c].b_name,name.c_str());
                                                                        tiAux.i_mtime = time(nullptr);
                                                                        fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                        fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                        cout << "Proceso Finalizado con Exito" << endl << endl;
                                                                    }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                                    goto t0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        cout << "Ruta no encontrada" << endl << endl;
                    }

                    t0:
                    fseek(archivo, ubicacion, SEEK_SET);
                    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                }
            }

            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}


void GestorArchivos::mkdir(string path, bool r) {
    if(this->usuario->nombreG == "" && this->usuario->nombreU == ""){
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount * nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            if (this->verificarPath(path)) {
                path = path.erase(0, 1);
                vector<string> ficheros = this->split(path, '/');
                string newCarpeta = ficheros.back();
                ficheros.pop_back();

                this->buscarficheroMkdir(ficheros, newCarpeta, r, sb, inicioSB, sb.s_inode_start, archivo);
            }

            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

//Realizar acciones del comando mkdir
void GestorArchivos::buscarficheroMkdir(vector<string> &ficheros, string newCarpeta, bool r, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);
    bool escritura = false, lectura = false;
    this->verificarPermisos(ti, escritura, lectura);

    if (ficheros.size() > 0) {
        if (ti.i_type == '0') {
            string fichero = ficheros[0];
            ficheros.erase(ficheros.begin());
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, fichero);

            if (ubicacion != -1) {
                this->buscarficheroMkdir(ficheros, newCarpeta, r, sb, inicioSB, ubicacion, archivo);

            } else if (ubicacion == -1 && r) {
                if (escritura) {
                    ubicacion = this->buscarEspacioCarpeta(ti, inicioInodo, archivo, fichero, sb, inicioSB);
                    if (ubicacion != -1) {
                        cout << "Fichero Creado: " << fichero << endl << endl;
                        this->buscarficheroMkdir(ficheros, newCarpeta, r, sb, inicioSB, ubicacion, archivo);

                        ti.i_mtime = time(nullptr);
                        fseek(archivo, inicioInodo, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    } else {
                        cout << endl;
                        return;
                    }
                } else { cout << "El usuario no tiene permisos de Escritura" << endl << endl; }
            } else { cout << "No se encontro el fichero" << fichero << endl << endl; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
    } else{
        if (ti.i_type == '0') {
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, newCarpeta);

            if (ubicacion != -1) {
                cout << "La carpeta ya existe" << endl << endl;

            } else {
                if (escritura) {
                    ubicacion = this->buscarEspacioCarpeta(ti, inicioInodo, archivo, newCarpeta, sb, inicioSB);
                    if (ubicacion != -1) {
                        cout << "Fichero Creado: " << newCarpeta << endl << endl;

                        ti.i_mtime = time(nullptr);
                        fseek(archivo, inicioInodo, SEEK_SET);
                        fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                    } else {
                        cout << endl;
                        return;
                    }
                } else { cout << "El usuario no tiene permisos de Escritura" << endl << endl; }
            }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; }
    }
}


void GestorArchivos::move(string path, string destino) {
    if (this->usuario->nombreG == "" && this->usuario->nombreU == "") {
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount *nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            regex expresion(
                    "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
            if (!regex_match(path, expresion) || !this->verificarPath(destino)) {
                cout << "Ruta en path y/o en destino no valida " << endl << endl;
            } else {
                path = path.erase(0, 1);
                vector<string> ficheros = this->split(path, '/');

                int ubicacion = this->buscarficheroRemove(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                if(ubicacion != -1){
                    TablaInodo ti;
                    fseek(archivo, ubicacion, SEEK_SET);
                    fread(&ti, sizeof(TablaInodo), 1, archivo);

                    if(ti.i_type == '1'){
                        cout << "El nombre no correponde a una carpeta" << endl << endl;
                    }
                    else if (ti.i_type == '0') {
                        BloqueApuntadores bap, bap1, bap2;
                        BloqueCarpeta bc;
                        TablaInodo tiAux;
                        char vacio = '\0', cero = '0';
                        for (int i = 0; i < 15; i++) {
                            if (ti.i_block[i] != -1) {
                                if (i == 0) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 2; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                            bool escritura, lectura;
                                            this->verificarPermisos(tiAux,escritura,lectura);

                                            if(escritura) {
                                                vector<string> ficherosD = this->split(destino, '/');
                                                if(this->buscarficheroMove(ficherosD,sb,inicioSB,sb.s_inode_start,archivo,bc.b_content[c].b_inodo,ficheros[0])){}
                                                tiAux.i_mtime = time(nullptr);
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);

                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                bc.b_content[c].b_inodo = -1;
                                                fseek(archivo, ti.i_block[i], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                            } else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                            goto t0;
                                        }
                                    }
                                }
                                else if (i < 12) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 0; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                            bool escritura, lectura;
                                            this->verificarPermisos(tiAux,escritura,lectura);

                                            if(escritura) {
                                                vector<string> ficherosD = this->split(destino, '/');
                                                if(this->buscarficheroMove(ficherosD,sb,inicioSB,sb.s_inode_start,archivo,bc.b_content[c].b_inodo,ficheros[0])){}
                                                tiAux.i_mtime = time(nullptr);
                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);

                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                bc.b_content[c].b_inodo = -1;
                                                fseek(archivo, ti.i_block[i], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                            }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                            goto t0;
                                        }
                                    }
                                }
                                else if (i == 12) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int c = 0; c < 4; c++) {
                                                if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                    fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                    fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                    bool escritura, lectura;
                                                    this->verificarPermisos(tiAux,escritura,lectura);

                                                    if(escritura) {
                                                        vector<string> ficherosD = this->split(destino, '/');
                                                        if(this->buscarficheroMove(ficherosD,sb,inicioSB,sb.s_inode_start,archivo,bc.b_content[c].b_inodo,ficheros[0])){}
                                                        tiAux.i_mtime = time(nullptr);
                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                        fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);

                                                        for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                        bc.b_content[c].b_inodo = -1;
                                                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                        cout << "Proceso Finalizado con Exito" << endl << endl;
                                                    }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                    goto t0;
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (i == 13) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                            for (int k = 0; k < 16; k++) {
                                                if (bap1.b_pointers[k] != -1) {
                                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                    for (int c = 0; c < 4; c++) {
                                                        if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                            fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                            fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                            bool escritura, lectura;
                                                            this->verificarPermisos(tiAux,escritura,lectura);

                                                            if(escritura) {
                                                                vector<string> ficherosD = this->split(destino, '/');
                                                                if(this->buscarficheroMove(ficherosD,sb,inicioSB,sb.s_inode_start,archivo,bc.b_content[c].b_inodo,ficheros[0])){}
                                                                tiAux.i_mtime = time(nullptr);
                                                                fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);

                                                                for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                                bc.b_content[c].b_inodo = -1;
                                                                fseek(archivo, bap.b_pointers[k], SEEK_SET);
                                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                                cout << "Proceso Finalizado con Exito" << endl << endl;
                                                            }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                            goto t0;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (i == 14) {
                                    fseek(archivo, ti.i_block[i], SEEK_SET);
                                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int j = 0; j < 16; j++) {
                                        if (bap.b_pointers[j] != -1) {
                                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                            for (int k = 0; k < 16; k++) {
                                                if (bap1.b_pointers[k] != -1) {
                                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                                    fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                                    for (int l = 0; l < 16; l++) {
                                                        if (bap2.b_pointers[l] != -1) {
                                                            fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                                            for (int c = 0; c < 4; c++) {
                                                                if (bc.b_content[c].b_inodo != -1 && strncmp(ficheros[0].c_str(),bc.b_content[c].b_name,12) == 0) {
                                                                    fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                    fread(&tiAux, sizeof(TablaInodo), 1, archivo);
                                                                    bool escritura, lectura;
                                                                    this->verificarPermisos(tiAux,escritura,lectura);

                                                                    if(escritura) {
                                                                        vector<string> ficherosD = this->split(destino, '/');
                                                                        if(this->buscarficheroMove(ficherosD,sb,inicioSB,sb.s_inode_start,archivo,bc.b_content[c].b_inodo,ficheros[0])){}
                                                                        tiAux.i_mtime = time(nullptr);
                                                                        fseek(archivo, bc.b_content[c].b_inodo, SEEK_SET);
                                                                        fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);

                                                                        for(int v = 0 ; v < 12; v++){ bc.b_content[c].b_name[v] = vacio; };
                                                                        bc.b_content[c].b_inodo = -1;
                                                                        fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                                                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                                        cout << "Proceso Finalizado con Exito" << endl << endl;
                                                                    }else { cout << "El usuario no posee permisos de escritura en la carpeta/archivo" << endl << endl; }
                                                                    goto t0;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        cout << "Ruta no encontrada" << endl << endl;
                    }

                    t0:
                    fseek(archivo, ubicacion, SEEK_SET);
                    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
                }
            }

            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

bool GestorArchivos::buscarficheroMove(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo,
                                       FILE *archivo, int inicioMove, string nombreMove) {
    TablaInodo ti;

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    if (ficheros.size() > 0) {
        if (ti.i_type == '0') {
            string fichero = ficheros[0];
            ficheros.erase(ficheros.begin());
            int ubicacion = this->buscarEnCarpeta(ti, inicioInodo, archivo, fichero);

            if (ubicacion != -1) {
                return this->buscarficheroMove(ficheros,sb,inicioSB,ubicacion,archivo,inicioMove,nombreMove);

            } else { cout << "No se encontro el fichero " << fichero << endl << endl; return false; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; return false; }
    }
    else {
        if (ti.i_type == '0') {
            return this->buscarEspacio(ti,inicioInodo,sb,inicioSB,archivo,inicioMove,nombreMove);
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; return false; }
    }
}

bool GestorArchivos::buscarEspacio(TablaInodo &ti, int inicioInodo, SuperBloque &sb, int inicioSB, FILE *archivo, int inicioMove, string nombreMove) {
    BloqueCarpeta bc;
    BloqueApuntadores bap, bap1, bap2;
    bool bandera = true, banderaE = false;
    char caracter = '1';
    for (int i = 0; i <= 11; i++) {
        if (bandera) {
            if (ti.i_block[i] != -1) {
                fseek(archivo, ti.i_block[i], SEEK_SET);
                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                for (int j = 0; j < 4; j++) {
                    if (bc.b_content[j].b_inodo == -1 && bandera) {
                        strcpy(bc.b_content[j].b_name, nombreMove.c_str());
                        bc.b_content[j].b_inodo = inicioMove;
                        bandera = false;
                    }
                }

                fseek(archivo, ti.i_block[i], SEEK_SET);
                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            }
            else if(bandera){
                if(sb.s_free_blocks_count > 1) {
                    //Bloque carpeta
                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                    ti.i_block[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                    sb.s_free_blocks_count--;

                    bc.b_content[0].b_inodo = inicioMove;
                    strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                    for (int j = 1; j < 4; j++) {
                        bc.b_content[j].b_inodo = -1;
                        strcpy(bc.b_content[j].b_name, "");
                    }

                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                    bandera = false;
                }
                else {
                    cout << "No es posible crear mas inodos y/o bloques"
                         << endl << endl;
                    bandera = false;
                    banderaE = true;
                    break;
                }
            }
        }
        else { break; }
    }

    if(ti.i_block[12] != -1 && bandera){
        fseek(archivo, ti.i_block[12], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int j = 0; j < 4; j++) {
                        if (bc.b_content[j].b_inodo == -1 && bandera) {
                            strcpy(bc.b_content[j].b_name, nombreMove.c_str());
                            bc.b_content[j].b_inodo = inicioMove;
                            bandera = false;
                        }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                }
                else if(bandera){
                    if(sb.s_free_blocks_count > 0) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        bc.b_content[0].b_inodo = inicioMove;
                        strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        banderaE = true;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[12], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[12] == -1 && bandera){
        if(sb.s_free_blocks_count > 1) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[12] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[12], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            bc.b_content[0].b_inodo = inicioMove;
            strcpy(bc.b_content[0].b_name, nombreMove.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
            banderaE = true;
        }
    }

    if(ti.i_block[13] != -1 && bandera){
        fseek(archivo, ti.i_block[13], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (bandera) {
                            if (bap1.b_pointers[j] != -1) {
                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                for (int k = 0; k < 4; k++) {
                                    if (bc.b_content[k].b_inodo == -1 && bandera) {
                                        strcpy(bc.b_content[k].b_name, nombreMove.c_str());
                                        bc.b_content[k].b_inodo = inicioMove;
                                        bandera = false;
                                    }
                                }

                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                            }
                            else if(bandera){
                                if(sb.s_free_blocks_count > 0) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    bc.b_content[0].b_inodo = inicioMove;
                                    strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                                    for (int k = 1; k < 4; k++) {
                                        bc.b_content[k].b_inodo = -1;
                                        strcpy(bc.b_content[k].b_name, "");
                                    }

                                    fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(bandera){
                    if(sb.s_free_blocks_count > 1) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int j = 1; j < 16; j++) { bap1.b_pointers[j] = -1; }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        bc.b_content[0].b_inodo = inicioMove;
                        strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap1.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        banderaE = true;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[13], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[13] == -1 && bandera){
        if(sb.s_free_blocks_count > 2) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[13] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[13], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = inicioMove;
            strcpy(bc.b_content[0].b_name, nombreMove.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
            banderaE = true;
        }
    }

    if(ti.i_block[14] != -1 &&  bandera){
        fseek(archivo, ti.i_block[14], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int a = 0; a < 16; a++){
            if (bandera) {
                if (bap.b_pointers[a] != -1) {
                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int i = 0; i < 16; i++){
                        if (bandera) {
                            if (bap1.b_pointers[i] != -1) {
                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                for(int j = 0; j < 16; j++){
                                    if (bandera) {
                                        if (bap2.b_pointers[j] != -1) {
                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int k = 0; k < 4; k++) {
                                                if (bc.b_content[k].b_inodo == -1 && bandera) {
                                                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                                    strcpy(bc.b_content[k].b_name, nombreMove.c_str());
                                                    bc.b_content[k].b_inodo = inicioMove;
                                                    bandera = false;
                                                }
                                            }

                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                        }
                                        else if(bandera){
                                            if(sb.s_free_blocks_count > 0) {
                                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                                bap2.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                                fwrite(&caracter, sizeof(caracter), 1, archivo);
                                                sb.s_free_blocks_count--;

                                                bc.b_content[0].b_inodo = inicioMove;
                                                strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                                                for (int k = 1; k < 4; k++) {
                                                    bc.b_content[k].b_inodo = -1;
                                                    strcpy(bc.b_content[k].b_name, "");
                                                }

                                                fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                bandera = false;
                                            }
                                            else {
                                                cout << "No es posible crear mas inodos y/o bloques"
                                                     << endl << endl;
                                                bandera = false;
                                                banderaE = true;
                                                break;
                                            }
                                        }
                                    }
                                    else { break; }
                                }

                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                            }
                            else if(bandera){
                                if(sb.s_free_blocks_count > 1) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    for (int j = 1; j < 16; j++) { bap2.b_pointers[j] = -1; }

                                    fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    bc.b_content[0].b_inodo = inicioMove;
                                    strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                                    for (int j = 1; j < 4; j++) {
                                        bc.b_content[j].b_inodo = -1;
                                        strcpy(bc.b_content[j].b_name, "");
                                    }

                                    fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                    bandera = false;
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    banderaE = true;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(bandera){
                    if(sb.s_free_blocks_count > 2) {
                        //Bloque apuntador
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[a] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[a], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque apuntador 2
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[0], SEEK_SET);
                        fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque Carpeta
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        bc.b_content[0].b_inodo = inicioMove;
                        strcpy(bc.b_content[0].b_name, nombreMove.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        banderaE = true;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[14], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[14] == -1 && bandera){
        if(sb.s_free_blocks_count > 3) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[14] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[14], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 3
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            bc.b_content[0].b_inodo = inicioMove;
            strcpy(bc.b_content[0].b_name, nombreMove.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap2.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
            banderaE = true;
        }
    }

    if (!bandera && !banderaE) {
        ti.i_mtime = time(nullptr);

        fseek(archivo, inicioInodo, SEEK_SET);
        fwrite(&ti, sizeof(TablaInodo), 1, archivo);

        sb.s_first_blo = this->buscarBM_b(sb, archivo);
        fseek(archivo, inicioSB, SEEK_SET);
        fwrite(&sb, sizeof(SuperBloque), 1, archivo);

        TablaInodo tiAux;
        fseek(archivo, inicioMove, SEEK_SET);
        fread(&tiAux, sizeof(TablaInodo), 1, archivo);

        fseek(archivo, tiAux.i_block[0], SEEK_SET);
        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

        bc.b_content[1].b_inodo = inicioInodo;

        fseek(archivo, tiAux.i_block[0], SEEK_SET);
        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);

        tiAux.i_mtime = time(nullptr);
        fseek(archivo, inicioMove, SEEK_SET);
        fwrite(&tiAux, sizeof(TablaInodo), 1, archivo);
        return true;
    }

    else if(bandera){
        cout << "No fue posible encontrar un espacio disponible en carpeta" << endl << endl;
    }
    return false;
}


void GestorArchivos::find(string path, string name) {
    if (this->usuario->nombreG == "" && this->usuario->nombreU == "") {
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount *nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            regex expresion(
                    "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
            if (!this->verificarPath(path)) {
                cout << "Ruta en path no valida " << endl << endl;
            } else {
                path = path.erase(0, 1);
                vector<string> ficheros = this->split(path, '/');

                int ubicacion = this->buscarficheroRemove(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                if(ubicacion != -1){
                    TablaInodo ti;
                    fseek(archivo, ubicacion, SEEK_SET);
                    fread(&ti, sizeof(TablaInodo), 1, archivo);

                    if(ti.i_type == '1'){
                        cout << "El nombre no correponde a una carpeta" << endl << endl;
                    }
                    else if (ti.i_type == '0') {

                        string sRegex = "";
                        for(int i = 0; i < name.size(); i++){
                            if(name[i] != '*' && name[i] != '?' && name[i] != '.'){
                                sRegex += name[i];
                            }else if(name[i] == '*'){
                                sRegex += "(.)*";
                            }else if(name[i] == '.'){
                                sRegex += "\\.";
                            }else if(name[i] == '?'){
                                sRegex += "(.)";
                            }
                        }

                        if(ficheros.size() > 0){
                            this->buscarEnCarpetaFind(ubicacion,sb,inicioSB,archivo,"/"+ficheros[0],0, sRegex);
                        }
                        else{
                            this->buscarEnCarpetaFind(ubicacion,sb,inicioSB,archivo,"/",0,sRegex);
                        }
                    }
                }
            }
            cout << endl << endl;
            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

void GestorArchivos::buscarEnCarpetaFind(int inicioInodo, SuperBloque &sb, int inicioSB, FILE *archivo, string nombre, int identacion, string sRegex) {
    TablaInodo ti;
    BloqueApuntadores bap, bap1, bap2;

    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);
    string sIdent = "";
    for (int s = 0; s < identacion; s++){ sIdent += " "; }

    bool escritura, lectura;
    this->verificarPermisos(ti, escritura, lectura);
    regex reg (sRegex);
    if (ti.i_type == '1' && lectura && regex_match(nombre, reg)) {
        cout << sIdent << "*" << nombre << endl;
    }
    else if (ti.i_type == '0' && lectura) {
        cout << sIdent << "-" << nombre << endl;
        BloqueApuntadores bap, bap1, bap2;
        BloqueCarpeta bc;
        TablaInodo tiAux;

        for (int i = 0; i < 15; i++) {
            if (ti.i_block[i] != -1) {
                if (i == 0) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int c = 2; c < 4; c++) {
                        if (bc.b_content[c].b_inodo != -1) {
                            string sigNombre = "";
                            sigNombre += bc.b_content[c].b_name;
                            this->buscarEnCarpetaFind(bc.b_content[c].b_inodo, sb, inicioSB, archivo, sigNombre, identacion+1, sRegex);
                        }
                    }
                } else if (i < 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int c = 0; c < 4; c++) {
                        if (bc.b_content[c].b_inodo != -1) {
                            string sigNombre = "";
                            sigNombre += bc.b_content[c].b_name;
                            this->buscarEnCarpetaFind(bc.b_content[c].b_inodo, sb, inicioSB, archivo, sigNombre,identacion+1, sRegex);
                        }
                    }
                } else if (i == 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                            for (int c = 0; c < 4; c++) {
                                if (bc.b_content[c].b_inodo != -1) {
                                    string sigNombre = "";
                                    sigNombre += bc.b_content[c].b_name;
                                    this->buscarEnCarpetaFind(bc.b_content[c].b_inodo, sb, inicioSB, archivo, sigNombre,identacion+1, sRegex);
                                }
                            }
                        }
                    }
                } else if (i == 13) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for (int k = 0; k < 16; k++) {
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 0; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1) {
                                            string sigNombre = "";
                                            sigNombre += bc.b_content[c].b_name;
                                            this->buscarEnCarpetaFind(bc.b_content[c].b_inodo, sb, inicioSB, archivo, sigNombre,identacion+1,sRegex);
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else if (i == 14) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for (int k = 0; k < 16; k++) {
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int l = 0; l < 16; l++) {
                                        if (bap2.b_pointers[l] != -1) {
                                            fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int c = 0; c < 4; c++) {
                                                if (bc.b_content[c].b_inodo != -1) {
                                                    string sigNombre = "";
                                                    sigNombre += bc.b_content[c].b_name;
                                                    this->buscarEnCarpetaFind(bc.b_content[c].b_inodo, sb, inicioSB, archivo, sigNombre,identacion+1,sRegex);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


void GestorArchivos::chown(string path, string usr, bool r) {
    if (this->usuario->nombreG == "" && this->usuario->nombreU == "") {
        cout << "No existe una sesion iniciada" << endl << endl;
        return;
    }
    NodoMount *nodo = this->listaMount->buscar(this->usuario->idParticion);
    if (nodo != NULL) {
        FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
        if (archivo != NULL) {
            SuperBloque sb;
            int inicioSB = 0;

            //Particion Primaria
            if (nodo->part_type == 'P') {
                MBR mbr;
                fseek(archivo, 0, SEEK_SET);
                fread(&mbr, sizeof(MBR), 1, archivo);
                int i;

                //Verificar la existencia de la particion
                for (i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                        inicioSB = mbr.mbr_partition_[i].part_start;
                        break;
                    }
                }

                //Error de posicion no encontrada
                if (i == 5) {
                    listaMount->eliminar(nodo->idCompleto);
                    cout << "No fue posible encontrar la particion en el disco" << endl << endl;
                    fclose(archivo);
                    return;
                }

                    //Posicion si Encontrada
                else {
                    if (mbr.mbr_partition_[i].part_status != '2') {
                        cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                        fclose(archivo);
                        return;
                    }
                    //Recuperar la informacion del superbloque
                    fseek(archivo, mbr.mbr_partition_[i].part_start, SEEK_SET);
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }
            }

                //Particiones Logicas
            else if (nodo->part_type == 'L') {
                EBR ebr;
                fseek(archivo, nodo->part_start, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                if (ebr.part_status != '2') {
                    cout << "No se ha aplicado el comando mkfs a la particion" << endl << endl;
                    fclose(archivo);
                    return;
                }
                inicioSB = nodo->part_start;
                fread(&sb, sizeof(SuperBloque), 1, archivo);
            }

            string utxt = this->getContentF(sb.s_inode_start + sizeof(TablaInodo), archivo);
            vector<string> grupos;
            vector<string> usuarios;
            this->obtenerUG(utxt, usuarios, grupos);

            int u = -1, g = -1;
            string sg = "";

            for(int i = 0; i < usuarios.size(); i++){
                vector<string> datos = this->split(usuarios[i],',');
                if(datos[0] != "0" && datos[3] == usr){
                    u = stoi(datos[0]);
                    sg = datos[2];
                    break;
                }
            }

            for(int i = 0; i < grupos.size(); i++){
                vector<string> datos = this->split(grupos[i],',');
                if(datos[0] != "0" && datos[2] == sg){
                    g = stoi(datos[0]);
                    break;
                }
            }

            if(u != -1 && g != -1) {
                regex expresion(
                        "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?)?");
                if (!this->verificarPath(path)) {
                    cout << "Ruta en path no valida " << endl << endl;
                } else {
                    path = path.erase(0, 1);
                    vector<string> ficheros = this->split(path, '/');

                    int ubicacion = this->buscarficheroRemove(ficheros, sb, inicioSB, sb.s_inode_start, archivo);

                    if (ubicacion != -1) {
                        TablaInodo ti;
                        fseek(archivo,ubicacion,SEEK_SET);
                        fread(&ti, sizeof(TablaInodo),1,archivo);
                        if(ti.i_type == '0') {
                            ubicacion = this->buscarEnCarpeta(ti,ubicacion,archivo,ficheros[0]);
                            if(ubicacion != -1) {
                                this->buscarEnCarpetaChown(ubicacion, sb, inicioSB, archivo, u, g, r);
                                cout << "Proceso finalizado" << endl << endl;
                            }else{ cout << "Fichero no Encontrado" << endl << endl;  }
                        }else { cout << "El directorio no correponde a una carpeta" << endl << endl; }
                    }
                }
            } else{ cout << "El usuario no existe" << endl << endl; }
            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

void GestorArchivos::buscarEnCarpetaChown(int inicioInodo, SuperBloque &sb, int inicioSB, FILE *archivo, int u, int g, bool r) {
    TablaInodo ti;
    BloqueApuntadores bap, bap1, bap2;

    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    bool escritura, lectura;
    this->verificarPermisos(ti, escritura, lectura);

    if (ti.i_type == '0' && lectura) {
        BloqueApuntadores bap, bap1, bap2;
        BloqueCarpeta bc;
        TablaInodo tiAux;

        for (int i = 0; i < 15; i++) {
            if (ti.i_block[i] != -1) {
                if (i == 0) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int c = 2; c < 4; c++) {
                        if (bc.b_content[c].b_inodo != -1 && r) {
                            this->buscarEnCarpetaChown(bc.b_content[c].b_inodo, sb, inicioSB, archivo,u,g,r);
                        }
                    }
                } else if (i < 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int c = 0; c < 4; c++) {
                        if (bc.b_content[c].b_inodo != -1 && r) {
                            this->buscarEnCarpetaChown(bc.b_content[c].b_inodo, sb, inicioSB, archivo,u,g,r);
                        }
                    }
                } else if (i == 12) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                            for (int c = 0; c < 4; c++) {
                                if (bc.b_content[c].b_inodo != -1 && r) {
                                    this->buscarEnCarpetaChown(bc.b_content[c].b_inodo, sb, inicioSB, archivo,u,g, r);
                                }
                            }
                        }
                    }
                } else if (i == 13) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for (int k = 0; k < 16; k++) {
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                    for (int c = 0; c < 4; c++) {
                                        if (bc.b_content[c].b_inodo != -1 && r) {
                                            this->buscarEnCarpetaChown(bc.b_content[c].b_inodo, sb, inicioSB, archivo,u,g,r);
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else if (i == 14) {
                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap.b_pointers[j] != -1) {
                            fseek(archivo, bap.b_pointers[j], SEEK_SET);
                            fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                            for (int k = 0; k < 16; k++) {
                                if (bap1.b_pointers[k] != -1) {
                                    fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                    fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int l = 0; l < 16; l++) {
                                        if (bap2.b_pointers[l] != -1) {
                                            fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int c = 0; c < 4; c++) {
                                                if (bc.b_content[c].b_inodo != -1 && r) {
                                                    this->buscarEnCarpetaChown(bc.b_content[c].b_inodo, sb, inicioSB, archivo,u,g,r);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if((ti.i_gid == this->usuario->idG && ti.i_uid == this->usuario->idU) || (this->usuario->nombreU == "root" && this->usuario->nombreG == "root")){
        ti.i_gid = g;
        ti.i_uid = u;
    }

    fseek(archivo, inicioInodo, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
}


//Obtener la informacion de un archivo en disco
string GestorArchivos::getContentF(int inicioInodo, FILE *archivo) {
    TablaInodo ti;
    BloqueArchivo ba;
    BloqueApuntadores bap, bap1, bap2;
    string contenido = "";

    //Obtener Inodo
    fseek(archivo, inicioInodo, SEEK_SET);
    fread(&ti, sizeof(TablaInodo), 1, archivo);

    //Recorrer Array de Bloques de inodo
    for (int i = 0; i < 15; i++) {
        if(ti.i_block[i] != -1){
            fseek(archivo,ti.i_block[i],SEEK_SET);

            //Bloques directos
            if(i<=11){
                fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                for(int j = 0; j < 64; j++){
                    if(ba.b_content[j] != '\0'){
                        contenido += ba.b_content[j];
                        continue;
                    }
                    break;
                }
            }

                //Bloque simple indirecto
            else if(i==12){
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if(bap.b_pointers[j] != -1){
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                        for(int k = 0; k < 64; k++){
                            if(ba.b_content[k] != '\0'){
                                contenido += ba.b_content[k];
                                continue;
                            }
                            break;
                        }
                    }
                }
            }

                //Bloque doble indirecto
            else if(i==13){
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if(bap.b_pointers[j] != -1){
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        for(int k = 0; k < 16; k++){
                            if(bap1.b_pointers[k] != -1){
                                fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                                for(int l = 0; l < 64; l++){
                                    if(ba.b_content[l] != '\0'){
                                        contenido += ba.b_content[l];
                                        continue;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }

                //Bloque triple indirecto
            else if(i==14){
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if(bap.b_pointers[j] != -1){
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        for(int k = 0; k < 16; k++){
                            if(bap1.b_pointers[k] != -1){
                                fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                                for(int l = 0; l < 16; l++){
                                    if(bap2.b_pointers[l] != -1){
                                        fseek(archivo, bap2.b_pointers[l], SEEK_SET);
                                        fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                                        for(int m = 0; m < 64; m++){
                                            if(ba.b_content[m] != '\0'){
                                                contenido += ba.b_content[m];
                                                continue;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ti.i_atime = time(nullptr);

    fseek(archivo, inicioInodo, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);
    return contenido;
}

//Buscar bloque libre en bitmap bloques
int GestorArchivos::buscarBM_b(SuperBloque &sb, FILE * archivo) {
    fseek(archivo, sb.s_bm_block_start, SEEK_SET);
    for(int i = 0; i < sb.s_blocks_count; i++){
        char caracter;
        fread(&caracter, sizeof(caracter), 1, archivo);
        if(caracter == '0'){
            return i;
        }
    }
    return -1;
}

//Buscar inodo libre en bitmap inodos
int GestorArchivos::buscarBM_i(SuperBloque &sb, FILE * archivo) {
    fseek(archivo, sb.s_bm_inode_start, SEEK_SET);
    for(int i = 0; i < sb.s_inodes_count; i++){
        char caracter;
        fread(&caracter, sizeof(caracter), 1, archivo);
        if(caracter == '0'){
            return i;
        }
    }
    return -1;
}

//Crear Carpetas
void GestorArchivos::crearCarpeta(FILE *archivo, SuperBloque &sb, int &ubicacion, int inicioInodo) {
    char caracter = '1';
    sb.s_first_blo = this->buscarBM_b(sb, archivo);
    sb.s_first_ino = this->buscarBM_i(sb, archivo);
    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
    fwrite(&caracter, sizeof(caracter), 1, archivo);
    fseek(archivo, sb.s_bm_inode_start + sb.s_first_ino, SEEK_SET);
    fwrite(&caracter, sizeof(caracter), 1, archivo);
    sb.s_free_inodes_count--;
    sb.s_free_blocks_count--;

    TablaInodo tCarpetaN;
    tCarpetaN.i_uid = this->usuario->idU;
    tCarpetaN.i_gid = this->usuario->idG;
    tCarpetaN.i_s = 0;
    tCarpetaN.i_atime = time(nullptr);
    tCarpetaN.i_ctime = time(nullptr);
    tCarpetaN.i_mtime = time(nullptr);
    tCarpetaN.i_block[0] =
            sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
    for (int i = 1; i < 15; i++) { tCarpetaN.i_block[i] = -1; }
    tCarpetaN.i_type = '0';
    tCarpetaN.i_perm = 664;

    fseek(archivo,
          sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo)),
          SEEK_SET);
    fwrite(&tCarpetaN, sizeof(TablaInodo), 1, archivo);

    BloqueCarpeta nbc;
    strcpy(nbc.b_content[0].b_name, ".");
    nbc.b_content[0].b_inodo =
            sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
    strcpy(nbc.b_content[1].b_name, "..");
    nbc.b_content[1].b_inodo = inicioInodo;
    strcpy(nbc.b_content[2].b_name, "");
    nbc.b_content[2].b_inodo = -1;
    strcpy(nbc.b_content[3].b_name, "");
    nbc.b_content[3].b_inodo = -1;
    fseek(archivo,
          sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta)),
          SEEK_SET);
    fwrite(&nbc, sizeof(BloqueCarpeta), 1, archivo);

    ubicacion =
            sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
}

//Crear Archivos
void GestorArchivos::crearArchivo(FILE *archivo, string textoArchivo, SuperBloque &sb, int inicioSB) {
    char caracter = '1';
    sb.s_first_ino = this->buscarBM_i(sb, archivo);
    fseek(archivo, sb.s_bm_inode_start + sb.s_first_ino, SEEK_SET);
    fwrite(&caracter, sizeof(caracter), 1, archivo);
    sb.s_free_inodes_count--;

    TablaInodo tiArchivo;
    tiArchivo.i_uid = this->usuario->idU;
    tiArchivo.i_gid = this->usuario->idG;
    tiArchivo.i_s = 0;
    tiArchivo.i_atime = time(nullptr);
    tiArchivo.i_ctime = time(nullptr);
    tiArchivo.i_mtime = time(nullptr);
    for (int i = 0; i < 15; i++) { tiArchivo.i_block[i] = -1; }
    tiArchivo.i_type = '1';
    tiArchivo.i_perm = 664;

    fseek(archivo,
          sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo)),
          SEEK_SET);
    fwrite(&tiArchivo, sizeof(TablaInodo), 1, archivo);

    this->writeInFile(textoArchivo, sb, inicioSB, sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo)), archivo);
}

//Buscar una carpeta o archivo en una carpeta padre
int GestorArchivos::buscarEnCarpeta(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre) {
    BloqueCarpeta bc;
    BloqueApuntadores bap, bap1, bap2;
    int ubicacion = -1;
    for (int i = 0; i < 15; i++) {
        if (ti.i_block[i] != -1) {
            fseek(archivo, ti.i_block[i], SEEK_SET);

            //Bloques directos
            if (i <= 11) {
                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);
                for (int j = 0; j < 4; j++) {
                    if (strncmp(bc.b_content[j].b_name, nombre.c_str(), 12) == 0) {
                        ubicacion = bc.b_content[j].b_inodo;
                    }
                }
            }

                //Bloque simple indirecto
            else if (i == 12) {
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if (bap.b_pointers[j] != -1) {
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        for (int k = 0; k < 64; k++) {
                            if (strncmp(bc.b_content[j].b_name, nombre.c_str(), 12) == 0) {
                                ubicacion = bc.b_content[j].b_inodo;
                            }
                        }
                    }
                }
            }

                //Bloque doble indirecto
            else if (i == 13) {
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if (bap.b_pointers[j] != -1) {
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        for (int k = 0; k < 16; k++) {
                            if (bap1.b_pointers[k] != -1) {
                                fseek(archivo, bap1.b_pointers[k], SEEK_SET);
                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                if (strncmp(bc.b_content[j].b_name, nombre.c_str(), 12) == 0) {
                                    ubicacion = bc.b_content[j].b_inodo;
                                }
                            }
                        }
                    }
                }
            }

                //Bloque triple indirecto
            else if (i == 14) {
                fread(&bap, sizeof(BloqueApuntadores), 1, archivo);
                for (int j = 0; j < 16; j++) {
                    if (bap.b_pointers[j] != -1) {
                        fseek(archivo, bap.b_pointers[j], SEEK_SET);
                        fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        for (int k = 0; k < 16; k++) {
                            if (bap1.b_pointers[k] != -1) {
                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                                for (int l = 0; l < 16; l++) {
                                    if (bap2.b_pointers[k] != -1) {
                                        fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                        fread(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                        if (strncmp(bc.b_content[j].b_name, nombre.c_str(), 12) == 0) {
                                            ubicacion = bc.b_content[j].b_inodo;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ti.i_atime = time(nullptr);
    fseek(archivo, inicioInodo, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);

    return ubicacion;
}

//Buscar un espacio libre en carpeta para crear una carpeta
int GestorArchivos::buscarEspacioCarpeta(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre, SuperBloque &sb,
                                         int inicioSB) {
    BloqueCarpeta bc;
    BloqueApuntadores bap, bap1, bap2;
    int ubicacion = -1;
    bool bandera = true;
    char caracter = '1';
    for (int i = 0; i <= 11; i++) {
        if (ubicacion == -1 && bandera) {
            if (ti.i_block[i] != -1) {
                fseek(archivo, ti.i_block[i], SEEK_SET);
                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                for (int j = 0; j < 4; j++) {
                    if (bc.b_content[j].b_inodo == -1 && ubicacion == -1 && bandera) {
                        if (sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                            sb.s_first_ino = this->buscarBM_i(sb, archivo);
                            strcpy(bc.b_content[j].b_name, nombre.c_str());
                            bc.b_content[j].b_inodo =
                                    sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                            this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                            bandera = false;
                        }
                        else {
                            cout << "No es posible crear mas inodos y/o bloques"
                                 << endl << endl;
                            bandera = false;
                            break;
                        }
                    }
                }

                fseek(archivo, ti.i_block[i], SEEK_SET);
                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            }
            else if(ubicacion == -1 && bandera){
                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                    //Bloque carpeta
                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                    ti.i_block[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                    sb.s_free_blocks_count--;

                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                    for (int j = 1; j < 4; j++) {
                        bc.b_content[j].b_inodo = -1;
                        strcpy(bc.b_content[j].b_name, "");
                    }

                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                    this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                    bandera = false;
                }
                else {
                    cout << "No es posible crear mas inodos y/o bloques"
                         << endl << endl;
                    bandera = false;
                    break;
                }
            }
        }
        else { break; }
    }

    if(ti.i_block[12] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[12], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int j = 0; j < 4; j++) {
                        if (bc.b_content[j].b_inodo == -1 && ubicacion == -1 && bandera) {
                            if (sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                                sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                strcpy(bc.b_content[j].b_name, nombre.c_str());
                                bc.b_content[j].b_inodo =
                                        sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                bandera = false;
                            }
                            else {
                                cout << "No es posible crear mas inodos y/o bloques"
                                     << endl << endl;
                                bandera = false;
                                break;
                            }
                        }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[12], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[12] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 2) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[12] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[12], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    if(ti.i_block[13] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[13], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (ubicacion == -1 && bandera) {
                            if (bap1.b_pointers[j] != -1) {
                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                for (int k = 0; k < 4; k++) {
                                    if (bc.b_content[k].b_inodo == -1 && ubicacion == -1 && bandera) {
                                        if (sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                                            sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                            strcpy(bc.b_content[k].b_name, nombre.c_str());
                                            bc.b_content[k].b_inodo =
                                                    sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                            this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                            bandera = false;
                                        }
                                        else {
                                            cout << "No es posible crear mas inodos y/o bloques"
                                                 << endl << endl;
                                            bandera = false;
                                            break;
                                        }
                                    }
                                }

                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                            }
                            else if(ubicacion == -1 && bandera){
                                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                                    for (int k = 1; k < 4; k++) {
                                        bc.b_content[k].b_inodo = -1;
                                        strcpy(bc.b_content[k].b_name, "");
                                    }

                                    fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                    this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                    bandera = false;
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 2) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int j = 1; j < 16; j++) { bap1.b_pointers[j] = -1; }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap1.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[13], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[13] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 3) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[13] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[13], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    if(ti.i_block[14] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[14], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int a = 0; a < 16; a++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[a] != -1) {
                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int i = 0; i < 16; i++){
                        if (ubicacion == -1 && bandera) {
                            if (bap1.b_pointers[i] != -1) {
                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                for(int j = 0; j < 16; j++){
                                    if (ubicacion == -1 && bandera) {
                                        if (bap2.b_pointers[j] != -1) {
                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int k = 0; k < 4; k++) {
                                                if (bc.b_content[k].b_inodo == -1 && ubicacion == -1 && bandera) {
                                                    if (sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                                                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                                        strcpy(bc.b_content[k].b_name, nombre.c_str());
                                                        bc.b_content[k].b_inodo =
                                                                sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                                        this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                                        bandera = false;
                                                    }
                                                    else {
                                                        cout << "No es posible crear mas inodos y/o bloques"
                                                             << endl << endl;
                                                        bandera = false;
                                                        break;
                                                    }
                                                }
                                            }

                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                        }
                                        else if(ubicacion == -1 && bandera){
                                            if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                                bap2.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                                fwrite(&caracter, sizeof(caracter), 1, archivo);
                                                sb.s_free_blocks_count--;

                                                sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                                bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                                strcpy(bc.b_content[0].b_name, nombre.c_str());
                                                for (int k = 1; k < 4; k++) {
                                                    bc.b_content[k].b_inodo = -1;
                                                    strcpy(bc.b_content[k].b_name, "");
                                                }

                                                fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                                bandera = false;
                                            }
                                            else {
                                                cout << "No es posible crear mas inodos y/o bloques"
                                                     << endl << endl;
                                                bandera = false;
                                                break;
                                            }
                                        }
                                    }
                                    else { break; }
                                }

                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                            }
                            else if(ubicacion == -1 && bandera){
                                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 2) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    for (int j = 1; j < 16; j++) { bap2.b_pointers[j] = -1; }

                                    fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                                    for (int j = 1; j < 4; j++) {
                                        bc.b_content[j].b_inodo = -1;
                                        strcpy(bc.b_content[j].b_name, "");
                                    }

                                    fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                    this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                                    bandera = false;
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 3) {
                        //Bloque apuntador
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[a] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[a], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque apuntador 2
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[0], SEEK_SET);
                        fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque Carpeta
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[14], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[14] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 4) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[14] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[14], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 3
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap2.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearCarpeta(archivo, sb, ubicacion, inicioInodo);
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    ti.i_mtime = time(nullptr);

    fseek(archivo, inicioInodo, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);

    sb.s_first_blo = this->buscarBM_b(sb, archivo);
    fseek(archivo, inicioSB, SEEK_SET);
    fwrite(&sb, sizeof(SuperBloque), 1, archivo);

    if(bandera){
        cout << "No fue posible encontrar un espacio disponible en carpeta" << endl << endl;
    }
    return ubicacion;
}

//Buscar un espacio libre en carpeta para crear un archivo
int GestorArchivos::buscarEspacioArchivo(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre, SuperBloque &sb,
                                         int inicioSB, string textoArchivo) {
    BloqueCarpeta bc;
    BloqueApuntadores bap, bap1, bap2;
    int ubicacion = -1;
    bool bandera = true;
    char caracter = '1';
    for (int i = 0; i <= 11; i++) {
        if (ubicacion == -1 && bandera) {
            if (ti.i_block[i] != -1) {
                fseek(archivo, ti.i_block[i], SEEK_SET);
                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                for (int j = 0; j < 4; j++) {
                    if (bc.b_content[j].b_inodo == -1 && ubicacion == -1 && bandera) {
                        if (sb.s_free_inodes_count > 0) {
                            sb.s_first_ino = this->buscarBM_i(sb, archivo);
                            strcpy(bc.b_content[j].b_name, nombre.c_str());
                            bc.b_content[j].b_inodo =
                                    sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                            this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                            bandera = false;
                        }
                        else {
                            cout << "No es posible crear mas inodos y/o bloques"
                                 << endl << endl;
                            bandera = false;
                            break;
                        }
                    }
                }

                fseek(archivo, ti.i_block[i], SEEK_SET);
                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            }
            else if(ubicacion == -1 && bandera){
                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                    //Bloque carpeta
                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                    ti.i_block[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                    sb.s_free_blocks_count--;

                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                    for (int j = 1; j < 4; j++) {
                        bc.b_content[j].b_inodo = -1;
                        strcpy(bc.b_content[j].b_name, "");
                    }

                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                    this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                    bandera = false;
                }
                else {
                    cout << "No es posible crear mas inodos y/o bloques"
                         << endl << endl;
                    bandera = false;
                    break;
                }
            }
        }
        else { break; }
    }

    if(ti.i_block[12] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[12], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                    for (int j = 0; j < 4; j++) {
                        if (bc.b_content[j].b_inodo == -1 && ubicacion == -1 && bandera) {
                            if (sb.s_free_inodes_count > 0) {
                                sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                strcpy(bc.b_content[j].b_name, nombre.c_str());
                                bc.b_content[j].b_inodo =
                                        sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                bandera = false;
                            }
                            else {
                                cout << "No es posible crear mas inodos y/o bloques"
                                     << endl << endl;
                                bandera = false;
                                break;
                            }
                        }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[12], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[12] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[12] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[12], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    if(ti.i_block[13] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[13], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int i = 0; i < 16; i++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int j = 0; j < 16; j++){
                        if (ubicacion == -1 && bandera) {
                            if (bap1.b_pointers[j] != -1) {
                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                for (int k = 0; k < 4; k++) {
                                    if (bc.b_content[k].b_inodo == -1 && ubicacion == -1 && bandera) {
                                        if (sb.s_free_inodes_count > 0) {
                                            sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                            strcpy(bc.b_content[k].b_name, nombre.c_str());
                                            bc.b_content[k].b_inodo =
                                                    sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                            this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                            bandera = false;
                                        }
                                        else {
                                            cout << "No es posible crear mas inodos y/o bloques"
                                                 << endl << endl;
                                            bandera = false;
                                            break;
                                        }
                                    }
                                }

                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                            }
                            else if(ubicacion == -1 && bandera){
                                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                                    for (int k = 1; k < 4; k++) {
                                        bc.b_content[k].b_inodo = -1;
                                        strcpy(bc.b_content[k].b_name, "");
                                    }

                                    fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                    this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                    bandera = false;
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int j = 1; j < 16; j++) { bap1.b_pointers[j] = -1; }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap1.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[13], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[13] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 2) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[13] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[13], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    if(ti.i_block[14] != -1 && ubicacion == -1 && bandera){
        fseek(archivo, ti.i_block[14], SEEK_SET);
        fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

        for(int a = 0; a < 16; a++){
            if (ubicacion == -1 && bandera) {
                if (bap.b_pointers[a] != -1) {
                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for(int i = 0; i < 16; i++){
                        if (ubicacion == -1 && bandera) {
                            if (bap1.b_pointers[i] != -1) {
                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                                for(int j = 0; j < 16; j++){
                                    if (ubicacion == -1 && bandera) {
                                        if (bap2.b_pointers[j] != -1) {
                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fread(&bc, sizeof(BloqueCarpeta), 1, archivo);

                                            for (int k = 0; k < 4; k++) {
                                                if (bc.b_content[k].b_inodo == -1 && ubicacion == -1 && bandera) {
                                                    if (sb.s_free_inodes_count > 0) {
                                                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                                        strcpy(bc.b_content[k].b_name, nombre.c_str());
                                                        bc.b_content[k].b_inodo =
                                                                sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                                        this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                                        bandera = false;
                                                    }
                                                    else {
                                                        cout << "No es posible crear mas inodos y/o bloques"
                                                             << endl << endl;
                                                        bandera = false;
                                                        break;
                                                    }
                                                }
                                            }

                                            fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                        }
                                        else if(ubicacion == -1 && bandera){
                                            if(sb.s_free_inodes_count > 0) {
                                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                                bap2.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueCarpeta));
                                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                                fwrite(&caracter, sizeof(caracter), 1, archivo);
                                                sb.s_free_blocks_count--;

                                                sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                                bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                                strcpy(bc.b_content[0].b_name, nombre.c_str());
                                                for (int k = 1; k < 4; k++) {
                                                    bc.b_content[k].b_inodo = -1;
                                                    strcpy(bc.b_content[k].b_name, "");
                                                }

                                                fseek(archivo, bap2.b_pointers[j], SEEK_SET);
                                                fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                                this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                                bandera = false;
                                            }
                                            else {
                                                cout << "No es posible crear mas inodos y/o bloques"
                                                     << endl << endl;
                                                bandera = false;
                                                break;
                                            }
                                        }
                                    }
                                    else { break; }
                                }

                                fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                            }
                            else if(ubicacion == -1 && bandera){
                                if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 0) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    for (int j = 1; j < 16; j++) { bap2.b_pointers[j] = -1; }

                                    fseek(archivo, bap1.b_pointers[i], SEEK_SET);
                                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);
                                    sb.s_free_blocks_count--;

                                    sb.s_first_ino = this->buscarBM_i(sb, archivo);
                                    bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                                    strcpy(bc.b_content[0].b_name, nombre.c_str());
                                    for (int j = 1; j < 4; j++) {
                                        bc.b_content[j].b_inodo = -1;
                                        strcpy(bc.b_content[j].b_name, "");
                                    }

                                    fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                                    fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                                    this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                                    bandera = false;
                                }
                                else {
                                    cout << "No es posible crear mas inodos y/o bloques"
                                         << endl << endl;
                                    bandera = false;
                                    break;
                                }
                            }
                        }
                        else { break; }
                    }

                    fseek(archivo, bap.b_pointers[a], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                }
                else if(ubicacion == -1 && bandera){
                    if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 1) {
                        //Bloque apuntador
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[a] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[a], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque apuntador 2
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

                        fseek(archivo, bap.b_pointers[0], SEEK_SET);
                        fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                        //Bloque Carpeta
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);
                        sb.s_free_blocks_count--;

                        sb.s_first_ino = this->buscarBM_i(sb, archivo);
                        bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
                        strcpy(bc.b_content[0].b_name, nombre.c_str());
                        for (int j = 1; j < 4; j++) {
                            bc.b_content[j].b_inodo = -1;
                            strcpy(bc.b_content[j].b_name, "");
                        }

                        fseek(archivo, bap2.b_pointers[0], SEEK_SET);
                        fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
                        this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
                        bandera = false;
                    }
                    else {
                        cout << "No es posible crear mas inodos y/o bloques"
                             << endl << endl;
                        bandera = false;
                        break;
                    }
                }
            }
            else { break; }
        }

        fseek(archivo, ti.i_block[14], SEEK_SET);
        fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
    }
    else if(ti.i_block[14] == -1 && ubicacion == -1 && bandera){
        if(sb.s_free_inodes_count > 0 && sb.s_free_blocks_count > 2) {
            //Bloque apuntador
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            ti.i_block[14] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap.b_pointers[i] = -1; }

            fseek(archivo, ti.i_block[14], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 2
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap1.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap1.b_pointers[i] = -1; }

            fseek(archivo, bap.b_pointers[0], SEEK_SET);
            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque apuntador 3
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            bap2.b_pointers[0] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
            for (int i = 1; i < 16; i++) { bap2.b_pointers[i] = -1; }

            fseek(archivo, bap1.b_pointers[0], SEEK_SET);
            fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);

            //Bloque Carpeta
            sb.s_first_blo = this->buscarBM_b(sb, archivo);
            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
            fwrite(&caracter, sizeof(caracter), 1, archivo);
            sb.s_free_blocks_count--;

            sb.s_first_ino = this->buscarBM_i(sb, archivo);
            bc.b_content[0].b_inodo = sb.s_inode_start + (sb.s_first_ino * sizeof(TablaInodo));
            strcpy(bc.b_content[0].b_name, nombre.c_str());
            for (int j = 1; j < 4; j++) {
                bc.b_content[j].b_inodo = -1;
                strcpy(bc.b_content[j].b_name, "");
            }

            fseek(archivo, bap2.b_pointers[0], SEEK_SET);
            fwrite(&bc, sizeof(BloqueCarpeta), 1, archivo);
            this->crearArchivo(archivo, textoArchivo, sb, inicioSB);
            bandera = false;
        }
        else {
            cout << "No es posible crear mas inodos y/o bloques"
                 << endl << endl;
            bandera = false;
        }
    }

    ti.i_mtime = time(nullptr);

    fseek(archivo, inicioInodo, SEEK_SET);
    fwrite(&ti, sizeof(TablaInodo), 1, archivo);

    sb.s_first_blo = this->buscarBM_b(sb, archivo);
    sb.s_first_ino = this->buscarBM_i(sb, archivo);
    fseek(archivo, inicioSB, SEEK_SET);
    fwrite(&sb, sizeof(SuperBloque), 1, archivo);

    if(bandera){
        cout << "No fue posible encontrar un espacio disponible en carpeta" << endl << endl;
    }
    return ubicacion;
}

void GestorArchivos::writeInFile(string texto, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
    if(texto.size() <= 280320) {
        TablaInodo ti;

        char caracter = '1';
        int nChar = 0;
        bool bandera = false;

        fseek(archivo, inicioInodo, SEEK_SET);
        fread(&ti, sizeof(TablaInodo), 1, archivo);
        ti.i_s = texto.size();

        //Recorrer Array de Bloques de inodo
        for (int i = 0; i < 12; i++) {
            if (ti.i_block[i] != -1) {
                BloqueArchivo ba;
                fseek(archivo, ti.i_block[i], SEEK_SET);
                fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                for (int j = 0; j < 64; j++) {
                    if (nChar < texto.size()) {
                        ba.b_content[j] = texto[nChar];
                        nChar++;
                        continue;
                    } else {
                        ba.b_content[j] = '\0';
                    }
                }
                fseek(archivo, ti.i_block[i], SEEK_SET);
                fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);
            } else if (ti.i_block[i] == -1 && nChar < texto.size() && !bandera) {
                if (sb.s_free_blocks_count > 0) {
                    BloqueArchivo ba;
                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                    ti.i_block[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                    for (int j = 0; j < 64; j++) {
                        if (nChar < texto.size()) {
                            ba.b_content[j] = texto[nChar];
                            nChar++;
                            continue;
                        } else {
                            ba.b_content[j] = '\0';
                        }
                    }

                    fseek(archivo, ti.i_block[i], SEEK_SET);
                    fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                    fwrite(&caracter, sizeof(caracter), 1, archivo);

                    sb.s_free_blocks_count--;
                } else {
                    cout << "No es posible crear mas bloques en la particion" << endl;
                    bandera = true;
                    break;
                }
            } else {
                break;
            }
        }

        if (ti.i_block[12] != -1) {
            BloqueArchivo ba;
            BloqueApuntadores bap;
            fseek(archivo, ti.i_block[12], SEEK_SET);
            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

            for (int i = 0; i < 16; i++) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&ba, sizeof(BloqueArchivo), 1, archivo);
                    for (int j = 0; j < 64; j++) {
                        if (nChar < texto.size()) {
                            ba.b_content[j] = texto[nChar];
                            nChar++;
                            continue;
                        } else {
                            ba.b_content[j] = '\0';
                        }
                    }
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);
                } else if (bap.b_pointers[i] == -1 && nChar < texto.size() && !bandera) {
                    if (sb.s_free_blocks_count > 0) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                        for (int j = 0; j < 64; j++) {
                            if (nChar < texto.size()) {
                                ba.b_content[j] = texto[nChar];
                                nChar++;
                                continue;
                            } else {
                                ba.b_content[j] = '\0';
                            }
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                        sb.s_free_blocks_count--;
                    } else {
                        cout << "No es posible crear mas bloques en la particion" << endl;
                        bandera = true;
                        break;
                    }
                }
            }

            fseek(archivo, ti.i_block[12], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
        } else if (ti.i_block[12] == -1 && nChar < texto.size() && !bandera) {
            if (sb.s_free_blocks_count > 0) {
                BloqueArchivo ba;
                BloqueApuntadores bap;
                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                ti.i_block[12] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                fwrite(&caracter, sizeof(caracter), 1, archivo);

                sb.s_free_blocks_count--;

                for (int i = 0; i < 16; i++) { bap.b_pointers[i] = -1; }

                for (int i = 0; i < 16; i++) {
                    if (nChar < texto.size()) {
                        if (sb.s_free_blocks_count > 0) {
                            sb.s_first_blo = this->buscarBM_b(sb, archivo);
                            bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                            for (int j = 0; j < 64; j++) {
                                if (nChar < texto.size()) {
                                    ba.b_content[j] = texto[nChar];
                                    nChar++;
                                    continue;
                                } else {
                                    ba.b_content[j] = '\0';
                                }
                            }

                            fseek(archivo, bap.b_pointers[i], SEEK_SET);
                            fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                            fwrite(&caracter, sizeof(caracter), 1, archivo);

                            sb.s_free_blocks_count--;
                        } else {
                            cout << "No es posible crear mas bloques en la particion" << endl;
                            bandera = true;
                            break;
                        }
                    } else { break; }
                }

                fseek(archivo, ti.i_block[12], SEEK_SET);
                fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
            } else {
                cout << "No es posible crear mas bloques en la particion" << endl;
                bandera = true;
            }
        }

        if (ti.i_block[13] != -1) {
            BloqueArchivo ba;
            BloqueApuntadores bap, bap1;
            fseek(archivo, ti.i_block[13], SEEK_SET);
            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

            for (int i = 0; i < 16; i++) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap1.b_pointers[j] != -1) {
                            fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                            fread(&ba, sizeof(BloqueArchivo), 1, archivo);

                            for (int k = 0; k < 64; k++) {
                                if (nChar < texto.size()) {
                                    ba.b_content[k] = texto[nChar];
                                    nChar++;
                                    continue;
                                } else {
                                    ba.b_content[k] = '\0';
                                }
                            }

                            fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                            fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);
                        } else if (bap1.b_pointers[j] == -1 && nChar < texto.size() && !bandera) {
                            if (sb.s_free_blocks_count > 0) {
                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                for (int k = 0; k < 64; k++) {
                                    if (nChar < texto.size()) {
                                        ba.b_content[k] = texto[nChar];
                                        nChar++;
                                        continue;
                                    } else {
                                        ba.b_content[k] = '\0';
                                    }
                                }

                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                fwrite(&caracter, sizeof(caracter), 1, archivo);

                                sb.s_free_blocks_count--;
                            } else {
                                cout << "No es posible crear mas bloques en la particion" << endl;
                                bandera = true;
                                break;
                            }
                        }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                } else if (bap.b_pointers[i] == -1 && nChar < texto.size() && !bandera) {
                    if (sb.s_free_blocks_count > 0) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                        sb.s_free_blocks_count--;

                        for (int j = 0; j < 16; j++) { bap1.b_pointers[j] = -1; }

                        for (int j = 0; j < 16; j++) {
                            if (nChar < texto.size()) {
                                if (sb.s_free_blocks_count > 0) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                    for (int k = 0; k < 64; k++) {
                                        if (nChar < texto.size()) {
                                            ba.b_content[k] = texto[nChar];
                                            nChar++;
                                            continue;
                                        } else {
                                            ba.b_content[k] = '\0';
                                        }
                                    }

                                    fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                    fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);

                                    sb.s_free_blocks_count--;
                                } else {
                                    cout << "No es posible crear mas bloques en la particion" << endl;
                                    bandera = true;
                                    break;
                                }
                            } else { break; }
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                    } else {
                        cout << "No es posible crear mas bloques en la particion" << endl;
                        bandera = true;
                        break;
                    }
                }
            }

            fseek(archivo, ti.i_block[13], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
        } else if (ti.i_block[13] == -1 && nChar < texto.size() && !bandera) {
            if (sb.s_free_blocks_count > 0) {
                BloqueArchivo ba;
                BloqueApuntadores bap, bap1;
                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                ti.i_block[13] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                fwrite(&caracter, sizeof(caracter), 1, archivo);

                sb.s_free_blocks_count--;

                for (int i = 0; i < 16; i++) { bap.b_pointers[i] = -1; }

                for (int i = 0; i < 16; i++) {
                    if (nChar < texto.size()) {
                        if (sb.s_free_blocks_count > 0) {
                            sb.s_first_blo = this->buscarBM_b(sb, archivo);
                            bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                            fwrite(&caracter, sizeof(caracter), 1, archivo);

                            sb.s_free_blocks_count--;

                            for (int j = 0; j < 16; j++) { bap1.b_pointers[j] = -1; }

                            for (int j = 0; j < 16; j++) {
                                if (nChar < texto.size()) {
                                    if (sb.s_free_blocks_count > 0) {
                                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                        bap1.b_pointers[j] =
                                                sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                        for (int k = 0; k < 64; k++) {
                                            if (nChar < texto.size()) {
                                                ba.b_content[k] = texto[nChar];
                                                nChar++;
                                                continue;
                                            } else {
                                                ba.b_content[k] = '\0';
                                            }
                                        }

                                        fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                        fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                                        sb.s_free_blocks_count--;
                                    } else {
                                        cout << "No es posible crear mas bloques en la particion" << endl;
                                        bandera = true;
                                        break;
                                    }
                                } else { break; }
                            }

                            fseek(archivo, bap.b_pointers[i], SEEK_SET);
                            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        } else {
                            cout << "No es posible crear mas bloques en la particion" << endl;
                            bandera = true;
                        }
                    }
                }

                fseek(archivo, ti.i_block[13], SEEK_SET);
                fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
            } else {
                cout << "No es posible crear mas bloques en la particion" << endl;
                bandera = true;
            }
        }

        if (ti.i_block[14] != -1) {
            BloqueArchivo ba;
            BloqueApuntadores bap, bap1, bap2;
            fseek(archivo, ti.i_block[14], SEEK_SET);
            fread(&bap, sizeof(BloqueApuntadores), 1, archivo);

            for (int i = 0; i < 16; i++) {
                if (bap.b_pointers[i] != -1) {
                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fread(&bap1, sizeof(BloqueApuntadores), 1, archivo);

                    for (int j = 0; j < 16; j++) {
                        if (bap1.b_pointers[j] != -1) {
                            fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                            fread(&bap2, sizeof(BloqueApuntadores), 1, archivo);

                            for (int k = 0; k < 16; k++) {
                                if (bap2.b_pointers[k] != -1) {
                                    fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                    fread(&ba, sizeof(BloqueApuntadores), 1, archivo);

                                    for (int l = 0; l < 64; l++) {
                                        if (nChar < texto.size()) {
                                            ba.b_content[l] = texto[nChar];
                                            nChar++;
                                            continue;
                                        } else {
                                            ba.b_content[l] = '\0';
                                        }
                                    }

                                    fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                    fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);
                                } else if (bap2.b_pointers[k] == -1 && nChar < texto.size() && !bandera) {
                                    if (sb.s_free_blocks_count > 0) {
                                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                        bap2.b_pointers[k] =
                                                sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                        for (int l = 0; l < 64; l++) {
                                            if (nChar < texto.size()) {
                                                ba.b_content[l] = texto[nChar];
                                                nChar++;
                                                continue;
                                            } else {
                                                ba.b_content[l] = '\0';
                                            }
                                        }

                                        fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                        fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                                        sb.s_free_blocks_count--;
                                    } else {
                                        cout << "No es posible crear mas bloques en la particion" << endl;
                                        bandera = true;
                                        break;
                                    }
                                }
                            }

                            fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                            fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                        } else if (bap1.b_pointers[j] == -1 && nChar < texto.size() && !bandera) {
                            if (sb.s_free_blocks_count > 0) {
                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                bap1.b_pointers[j] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                fwrite(&caracter, sizeof(caracter), 1, archivo);

                                sb.s_free_blocks_count--;

                                for (int k = 0; k < 16; k++) { bap2.b_pointers[k] = -1; }

                                for (int k = 0; k < 16; k++) {
                                    if (nChar < texto.size()) {
                                        if (sb.s_free_blocks_count > 0) {
                                            sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                            bap2.b_pointers[k] =
                                                    sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                            for (int l = 0; l < 64; l++) {
                                                if (nChar < texto.size()) {
                                                    ba.b_content[l] = texto[nChar];
                                                    nChar++;
                                                    continue;
                                                } else {
                                                    ba.b_content[l] = '\0';
                                                }
                                            }

                                            fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                            fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                            fwrite(&caracter, sizeof(caracter), 1, archivo);

                                            sb.s_free_blocks_count--;
                                        } else {
                                            cout << "No es posible crear mas bloques en la particion" << endl;
                                            bandera = true;
                                            break;
                                        }
                                    } else { break; }
                                }

                                fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                            } else {
                                cout << "No es posible crear mas bloques en la particion" << endl;
                                bandera = true;
                            }
                        }
                    }

                    fseek(archivo, bap.b_pointers[i], SEEK_SET);
                    fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                } else if (bap.b_pointers[i] == -1 && nChar < texto.size() && !bandera) {
                    if (sb.s_free_blocks_count > 0) {
                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                        bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                        sb.s_free_blocks_count--;

                        for (int j = 0; j < 16; j++) { bap1.b_pointers[j] = -1; }

                        for (int j = 0; j < 16; j++) {
                            if (nChar < texto.size()) {
                                if (sb.s_free_blocks_count > 0) {
                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                    bap1.b_pointers[j] =
                                            sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                    fwrite(&caracter, sizeof(caracter), 1, archivo);

                                    sb.s_free_blocks_count--;

                                    for (int k = 0; k < 16; k++) { bap2.b_pointers[k] = -1; }

                                    for (int k = 0; k < 16; k++) {
                                        if (nChar < texto.size()) {
                                            if (sb.s_free_blocks_count > 0) {
                                                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                                bap2.b_pointers[k] =
                                                        sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                                for (int l = 0; l < 64; l++) {
                                                    if (nChar < texto.size()) {
                                                        ba.b_content[l] = texto[nChar];
                                                        nChar++;
                                                        continue;
                                                    } else {
                                                        ba.b_content[l] = '\0';
                                                    }
                                                }

                                                fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                                fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                                fwrite(&caracter, sizeof(caracter), 1, archivo);

                                                sb.s_free_blocks_count--;
                                            } else {
                                                cout << "No es posible crear mas bloques en la particion" << endl;
                                                bandera = true;
                                                break;
                                            }
                                        } else { break; }
                                    }

                                    fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                    fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                                } else {
                                    cout << "No es posible crear mas bloques en la particion" << endl;
                                    bandera = true;
                                }
                            }
                        }

                        fseek(archivo, bap.b_pointers[i], SEEK_SET);
                        fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                    } else {
                        cout << "No es posible crear mas bloques en la particion" << endl;
                        bandera = true;
                    }
                }
            }

            fseek(archivo, ti.i_block[14], SEEK_SET);
            fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
        } else if (ti.i_block[14] == -1 && nChar < texto.size() && !bandera) {
            if (sb.s_free_blocks_count > 0) {
                BloqueArchivo ba;
                BloqueApuntadores bap, bap1, bap2;
                sb.s_first_blo = this->buscarBM_b(sb, archivo);
                ti.i_block[14] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                fwrite(&caracter, sizeof(caracter), 1, archivo);

                sb.s_free_blocks_count--;

                for (int i = 0; i < 16; i++) { bap.b_pointers[i] = -1; }

                for (int i = 0; i < 16; i++) {
                    if (nChar < texto.size()) {
                        if (sb.s_free_blocks_count > 0) {
                            sb.s_first_blo = this->buscarBM_b(sb, archivo);
                            bap.b_pointers[i] = sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                            fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                            fwrite(&caracter, sizeof(caracter), 1, archivo);

                            sb.s_free_blocks_count--;

                            for (int j = 0; j < 16; j++) { bap1.b_pointers[j] = -1; }

                            for (int j = 0; j < 16; j++) {
                                if (nChar < texto.size()) {
                                    if (sb.s_free_blocks_count > 0) {
                                        sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                        bap1.b_pointers[j] =
                                                sb.s_block_start + (sb.s_first_blo * sizeof(BloqueApuntadores));
                                        fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                        fwrite(&caracter, sizeof(caracter), 1, archivo);

                                        sb.s_free_blocks_count--;

                                        for (int k = 0; k < 16; k++) { bap2.b_pointers[k] = -1; }

                                        for (int k = 0; k < 16; k++) {
                                            if (nChar < texto.size()) {
                                                if (sb.s_free_blocks_count > 0) {
                                                    sb.s_first_blo = this->buscarBM_b(sb, archivo);
                                                    bap2.b_pointers[k] =
                                                            sb.s_block_start + (sb.s_first_blo * sizeof(BloqueArchivo));

                                                    for (int l = 0; l < 64; l++) {
                                                        if (nChar < texto.size()) {
                                                            ba.b_content[l] = texto[nChar];
                                                            nChar++;
                                                            continue;
                                                        } else {
                                                            ba.b_content[l] = '\0';
                                                        }
                                                    }

                                                    fseek(archivo, bap2.b_pointers[k], SEEK_SET);
                                                    fwrite(&ba, sizeof(BloqueArchivo), 1, archivo);

                                                    fseek(archivo, sb.s_bm_block_start + sb.s_first_blo, SEEK_SET);
                                                    fwrite(&caracter, sizeof(caracter), 1, archivo);

                                                    sb.s_free_blocks_count--;
                                                } else {
                                                    cout << "No es posible crear mas bloques en la particion" << endl;
                                                    bandera = true;
                                                    break;
                                                }
                                            } else { break; }
                                        }

                                        fseek(archivo, bap1.b_pointers[j], SEEK_SET);
                                        fwrite(&bap2, sizeof(BloqueApuntadores), 1, archivo);
                                    } else {
                                        cout << "No es posible crear mas bloques en la particion" << endl;
                                        bandera = true;
                                    }
                                }
                            }

                            fseek(archivo, bap.b_pointers[i], SEEK_SET);
                            fwrite(&bap1, sizeof(BloqueApuntadores), 1, archivo);
                        } else {
                            cout << "No es posible crear mas bloques en la particion" << endl;
                            bandera = true;
                        }
                    }
                }

                fseek(archivo, ti.i_block[14], SEEK_SET);
                fwrite(&bap, sizeof(BloqueApuntadores), 1, archivo);
            } else {
                cout << "No es posible crear mas bloques en la particion" << endl;
                bandera = true;
            }
        }

        ti.i_mtime = time(nullptr);
        fseek(archivo, inicioInodo, SEEK_SET);
        fwrite(&ti, sizeof(TablaInodo), 1, archivo);

        sb.s_first_blo = this->buscarBM_b(sb, archivo);
        sb.s_first_ino = this->buscarBM_i(sb, archivo);
        fseek(archivo, inicioSB, SEEK_SET);
        fwrite(&sb, sizeof(SuperBloque), 1, archivo);
        cout << "Modificacion Realizada" << endl << endl;
    }
    else{
        cout << "El texto que desea ingresar supear el limite del archivo" << endl << endl;
    }
}

//Verificar los permisos de escritura y lectura de un usuario en un inodo
void GestorArchivos::verificarPermisos(TablaInodo & inodo, bool &escritura, bool &lectura) {
    if(this->usuario->nombreG == "root" && this->usuario->nombreU == "root"){
        escritura = true;
        lectura = true;
        return;
    }

    int permisos = inodo.i_perm;
    int propietario = permisos / 100;
    permisos = permisos - (propietario * 100);
    int grupo = permisos / 10;
    permisos = permisos - (grupo * 10);
    int otros = permisos;

    if(this->usuario->idU == inodo.i_uid){
        if(propietario > 3){ lectura = true; }
        if(propietario == 2 || propietario == 3 || propietario == 6 || propietario == 7) { escritura = true; }
    }
    else if(this->usuario->idG == inodo.i_gid){
        if(grupo > 3){ lectura = true; }
        if(grupo == 2 || grupo == 3 || grupo == 6 || grupo == 7) { escritura = true; }
    }
    else{
        if(otros > 3){ lectura = true; }
        if(otros == 2 || otros == 3 || otros == 6 || otros == 7) { escritura = true; }
    }
}

//Verificar parametro ugo
bool GestorArchivos::verificarUGO(int ugo) {
    int propietario = ugo / 100;
    ugo = ugo - (propietario * 100);
    int grupo = ugo / 10;
    ugo = ugo - (grupo * 10);
    int otros = ugo;

    if((propietario <= 7 && propietario >= 0) && (grupo <= 7 && grupo >= 0) && (propietario <= 7 && propietario >= 0)){ return true; }
    return false;
}

//Verificar el parametro Cont
bool GestorArchivos::verificarArchivo(string cadena) {
    regex expresion(
            "[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\.[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)");
    if (!regex_match(cadena, expresion)) {
        cout << "La direccion ingresada en cont no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return false;
    }
    return true;
}

//Verificar el parametroPath
bool GestorArchivos::verificarPath(string cadena) {
    regex expresion(
            "(\\/)(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+)?");
    if (!regex_match(cadena, expresion)) {
        cout << "La direccion ingresada en path no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return false;
    }
    return true;
}

void GestorArchivos::obtenerUG(string cadena, vector<string> & usuarios, vector<string> & grupos) {
    vector<string> info = this->split(cadena, '\n');
    for (int i = 0; i < info.size(); i++) {
        vector<string> aux = this->split(info[i], ',');
        if (aux.size() == 3) {
            grupos.push_back(info[i]);
        } else if (aux.size() == 5) {
            usuarios.push_back(info[i]);
        }
    }
}

//Split string
vector<string> GestorArchivos::split(string cadena,char delim) {
    vector<string> cadenaSplit;
    string stringV;
    stringstream scadena(cadena);
    while (getline(scadena,stringV,delim)){
        if(stringV != "") {
            cadenaSplit.push_back(stringV);
        }
    }
    return cadenaSplit;
}
