//
// Created by estuardo on 16/02/23.
//

#include "AdminUsuario.h"
#include "Usuario.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

AdminUsuario::AdminUsuario(ListaMount * listaMount, Usuario * usuario) {
    this->listaMount = listaMount;
    this->usuario = usuario;
}

//Comado Login
void AdminUsuario::login(string usuario, string password, string id) {
    usuario = this->trim(usuario);
    password = this->trim(password);
    id = this->trim(id);
    if(this->usuario->idParticion == "") {
        if (usuario == "" || password == "" || id == "") {
            cout << "No fue posible ejecutar el comando con la informacion proporcionada" << endl << endl;
            return;
        }

        NodoMount *nodo = this->listaMount->buscar(id);

        if (nodo != NULL) {
            FILE *archivo = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb+");
            if (archivo != NULL) {
                SuperBloque sb;
                //Particion Primaria
                if (nodo->part_type == 'P') {
                    MBR mbr;
                    fseek(archivo, 0, SEEK_SET);
                    fread(&mbr, sizeof(MBR), 1, archivo);
                    int i;

                    //Verificar la existencia de la particion
                    for (i = 0; i < 4; i++) {
                        if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0) {
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
                    fread(&sb, sizeof(SuperBloque), 1, archivo);
                }

                //Acceder al archivo user.txt
                string utxt = this->getContentF(sb.s_inode_start + sizeof(TablaInodo), archivo);
                vector<string> usuarios;
                vector<string> grupos;
                this->obtenerUG(utxt, usuarios, grupos);

                for (int i = 0; i < usuarios.size(); i++) {
                    vector<string> uDatos = this->split(usuarios[i], ',');
                    if (uDatos[0] != "0" && uDatos[3] == usuario && uDatos[4] == password) {
                        for (int j = 0; j < grupos.size(); j++) {
                            vector<string> gDatos = this->split(grupos[j], ',');
                            if(uDatos[2] == gDatos[2] && gDatos[0] != "0") {
                                this->usuario->ingresarInfoU(stoi(gDatos[0]),stoi(uDatos[0]),uDatos[2],uDatos[3],uDatos[4],id);

                                cout << "Sesion iniciada correctamente" << endl << endl;
                                fclose(archivo);
                                return;
                            }
                        }
                    }
                }

                cout << "No fue posible iniciar sesion" << endl << endl;
                fclose(archivo);
            } else {
                listaMount->eliminar(nodo->idCompleto);
                cout << "No fue posible encontrar el disco de la particion" << endl << endl;
                return;
            }
        }
    }
    else{
        cout << "Ya existe una sesion iniciada" << endl << endl;
        return;
    }
}

//Comando Logout
void AdminUsuario::logout() {
    if(this->usuario->idParticion == ""){
        cout << "No hay una sesion iniciada con anterioridad" << endl << endl;
        return;
    }
    this->usuario->borrarInfoU();
    cout << "Sesion Cerrada" << endl << endl ;
}

//Comando Mkgrp
void AdminUsuario::mkgrp(string name) {
    name = this->trim(name);
    if(this->usuario->idParticion == ""){
        cout << "No hay una sesion iniciada con anterioridad" << endl << endl;
        return;
    }
    else if(this->usuario->nombreU != "root" && this->usuario->nombreG != "root"){
        cout << "El usuario actual no puede ejecutar el comando" << endl << endl;
        return;
    }
    else if(name.size() >= 10 || name.size() == 0){
        cout << "El nombre del grupo debe contener un maximo de 10 caracteres" << endl << endl;
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

            //Acceder al archivo user.txt
            string utxt = this->getContentF(sb.s_inode_start + sizeof(TablaInodo), archivo);
            vector<string> usuarios;
            vector<string> grupos;
            this->obtenerUG(utxt, usuarios, grupos);

            for(int i = 0; i < grupos.size(); i++){
                vector<string> gDatos = this->split(grupos[i],',');
                if(gDatos[2] == name && gDatos[0] != "0"){
                    cout << "El grupo ya existe" << endl << endl;
                    fclose(archivo);
                    return;
                }
            }

            int numeroG = grupos.size() +1;
            utxt += to_string(numeroG)+",G," + name + "\n";
            this->writeInFile(utxt, sb, inicioSB, sb.s_inode_start + sizeof(TablaInodo), archivo);
            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            return;
        }
    }
}

//Comando Rmgrp
void AdminUsuario::rmgrp(string name) {
    name = this->trim(name);
    if(this->usuario->idParticion == ""){
        cout << "No hay una sesion iniciada con anterioridad" << endl << endl;
        return;
    }
    else if(this->usuario->nombreU != "root" && this->usuario->nombreG != "root"){
        cout << "El usuario actual no puede ejecutar el comando" << endl << endl;
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

            //Acceder al archivo user.txt
            string utxt = this->getContentF(sb.s_inode_start + sizeof(TablaInodo), archivo);
            vector<string> grupos;
            vector<string> usuarios;
            this->obtenerUG(utxt, usuarios, grupos);

            bool bandera = false;
            for(int i = 0; i < grupos.size(); i++){
                vector<string> datos = this->split(grupos[i],',');
                if(datos[0] != "0" && datos[2] == name){
                    bandera = true;
                    break;
                }
            }

            if(bandera){
                vector<string> contenido = this->split(utxt,'\n');
                string final = "";
                for(int i = 0; i < contenido.size(); i++){
                    vector<string> datos = this->split(contenido[i],',');
                    if(datos.size() == 3){
                        if(datos[0] != "0" && datos[2] == name){
                            final += "0,"+datos[1]+","+datos[2]+"\n";
                        }
                        else{ final += contenido[i]+"\n"; }
                    }
                    else if(datos.size() == 5){ final += contenido[i]+"\n"; }
                }

                this->writeInFile(final, sb, inicioSB, sb.s_inode_start + sizeof(TablaInodo), archivo);
            }
            else{ cout << "El grupo que desea eliminar no existe" << endl << endl; }
            fclose(archivo);
        } else {
            listaMount->eliminar(nodo->idCompleto);
            cout << "No fue posible encontrar el disco de la particion" << endl << endl;
            fclose(archivo);
            return;
        }
    }
}

int AdminUsuario::buscarBM_b(SuperBloque &sb, FILE * archivo) {
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

//Escribir en un archivo
void AdminUsuario::writeInFile(string texto, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
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
        fseek(archivo, inicioSB, SEEK_SET);
        fwrite(&sb, sizeof(SuperBloque), 1, archivo);
        cout << "Modificacion Realizada" << endl << endl;
    }
    else{
        cout << "El texto que desea ingresar supear el limite del archivo" << endl << endl;
    }
}

//Obtener la informacion de un archivo en disco
string AdminUsuario::getContentF(int inicioInodo, FILE *archivo) {
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

//Separar en vectores los grupos y usuarios
void AdminUsuario::obtenerUG(string cadena, vector<string> & usuarios, vector<string> & grupos) {
    vector<string> info = this->split(cadena,'\n');
    for (int i = 0; i < info.size(); i++){
        vector<string> aux = this->split(info[i],',');
        if(aux.size() == 3){
            grupos.push_back(info[i]);
        }else if(aux.size() == 5){
            usuarios.push_back(info[i]);
        }
    }
}

//Funcion Split
vector<string> AdminUsuario::split(string cadena,char delim) {
    vector<string> cadenaSplit;
    string stringV;
    stringstream scadena(cadena);
    while (getline(scadena,stringV,delim)){
        cadenaSplit.push_back(stringV);
    }
    return cadenaSplit;
}

//Eliminar espacios en blacon al inicio y final
string AdminUsuario::trim(std::string cadena) {
    cadena.erase(cadena.begin(), std::find_if( cadena.begin(), cadena.end(),std::not1(std::ptr_fun<int, int>(std::isspace))));
    cadena.erase(std::find_if(cadena.rbegin(), cadena.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), cadena.end());
    return cadena;
}
