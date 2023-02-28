//
// Created by estuardo on 26/02/23.
//

#include <regex>
#include "GestorArchivos.h"

GestorArchivos::GestorArchivos(ListaMount *listaMount, Usuario *usuario) {
    this->listaMount = listaMount;
    this->usuario = usuario;
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