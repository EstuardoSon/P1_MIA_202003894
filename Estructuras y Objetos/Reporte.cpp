//
// Created by estuardo on 13/02/23.
//

#include "Reporte.h"
#include <iostream>
#include <regex>
#include <cmath>

Reporte::Reporte(ListaMount * listaMount) {
    this->listaMount = listaMount;
    this->name = "";
    this->fichero_p = "";
    this->archivo_p = "";
    this->fichero_r = "";
    this->archivo_r = "";
    this->id = "";
    this->ruta = "";
}

bool Reporte::verificarRuta(std::string fichero, std::string archivo, std::string extension) {
    regex expresion ("\\/(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\."+extension+")?)?");
    if (!regex_match(fichero+"/"+archivo,expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return false;
    }
    return true;
}

//Obtener Extension Archivo
string Reporte::obtenerExtension(std::string nombreArchivo) {
    string delimitador = ".";
    size_t pos = 0;
    string extension;
    while (pos = nombreArchivo.find(delimitador) != string::npos) {
        extension = nombreArchivo.erase(0, pos);
    }
    return extension;
}

//Generacion de Reportes
void Reporte::generarReporte() {
    if (this->name == "mbr"){
        this->reporteMBR();
    }
    else if (this->name == "disk"){
        this->reporteDisk();
    }
    else if(this->name == "sb"){
        this->reporteSb();
    }
    else if(this->name == "bm_inode"){
        this->reporteBmInode();
    }
    else if(this->name == "bm_bloc"){
        this->reporteBmBlock();
    }
    else if(this->name == "inode"){
        this->reporteInode();
    }
    else if(this->name == "block"){
        this->reporteBlock();
    }
    else if(this->name == "tree"){
        this->reporteTree();
    }
    else if(this->name == "ls"){
    }
    else if(this->name == "file"){
        this->reporteFile();
    }
}

//Generar Celdas de Particiones Primarias y Extendidas para Reporte MBR
void Reporte::crearParticionRMBR(FILE *archivoReporte, FILE *archivoDisco, Partition &partition) {
    fprintf(archivoReporte,
            "<tr><td colspan=\"2\" bgcolor=\"#4B0082\"><font color=\"white\">Particion</font></td></tr>\n");
    string cadena = "";
    cadena += partition.part_status;
    fprintf(archivoReporte,
            ("<tr><td color=\"white\">part_status</td><td color=\"white\">" + cadena + "</td></tr>\n").c_str());
    cadena = "";
    cadena += partition.part_type;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_type</td><td bgcolor="#6A5ACD" color="white">)" +
             cadena + "</td></tr>\n").c_str());
    cadena = "";
    cadena += partition.part_fit;
    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_fit</td><td color="white">)" +
                                   cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_start</td><td bgcolor="#6A5ACD" color="white">)" +
             to_string(partition.part_start) + "</td></tr>\n").c_str());

    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_s</td><td color="white">)" +
                                   to_string(partition.part_s) + "</td></tr>\n").c_str());

    string nombre = "";
    nombre += partition.part_name;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_name</td><td bgcolor="#6A5ACD" color="white">)" +
             nombre + "</td></tr>\n").c_str());

    if (partition.part_type == 'E') {
        this->crearParticionLRMBR(archivoReporte, archivoDisco, partition.part_start);
    }
}

//Generar Celdas de Particiones Logicas para Reporte MBR
void Reporte::crearParticionLRMBR(FILE *archivoReporte, FILE *archivoDisco, int part_start) {
    EBR ebr;
    fseek(archivoDisco, part_start, SEEK_SET);
    fread(&ebr, sizeof(EBR), 1, archivoDisco);

    fprintf(archivoReporte,
            "<tr><td colspan=\"2\" bgcolor=\"#D87093\"><font color=\"white\">Particion Logica</font></td></tr>\n");

    string cadena = "";
    cadena += ebr.part_status;
    fprintf(archivoReporte,
            ("<tr><td color=\"white\">part_status</td><td color=\"white\">" + cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_next</td><td bgcolor="#FFC0CB" color="white">)" +
             to_string(ebr.part_next) + "</td></tr>\n").c_str());
    cadena = "";
    cadena += ebr.part_fit;
    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_fit</td><td color="white">)" +
                                   cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_start</td><td bgcolor="#FFC0CB" color="white">)" +
             to_string(ebr.part_start) + "</td></tr>\n").c_str());

    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_s</td><td color="white">)" +
                                   to_string(ebr.part_s) + "</td></tr>\n").c_str());

    string nombre = "";
    nombre += ebr.part_name;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_name</td><td bgcolor="#FFC0CB" color="white">)" +
             nombre + "</td></tr>\n").c_str());

    if (ebr.part_next != -1) {
        this->crearParticionLRMBR(archivoReporte, archivoDisco, ebr.part_next);
    }
}

//Generar Reporte MBR
void Reporte::reporteMBR() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if(archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                FILE *archivoReporte = fopen((this->fichero_p + "/reporteMBR.dot").c_str(), "w+");
                fprintf(archivoReporte, "digraph G {\n");
                fprintf(archivoReporte, "node[shape=none]\n");
                fprintf(archivoReporte, "start[label=<<table>\n");
                fprintf(archivoReporte,
                        "<tr><td colspan=\"2\" bgcolor=\"#4B0082\"><font color=\"white\">REPORTE DE MBR</font></td></tr>\n");

                fprintf(archivoReporte,
                        (R"(<tr><td color="white">mbr_tamano</td><td color="white">)" + to_string(mbr.mbr_tamano) +
                         "</td></tr>\n").c_str());

                tm *fechaCreacion = localtime(&mbr.mbr_fecha_creacion);
                char buffer[128];
                strftime(buffer, sizeof(buffer), "%m-%d-%Y %X", fechaCreacion);
                fprintf(archivoReporte,
                        (R"(  <tr><td bgcolor="#6A5ACD" color="white">mbr_fecha_creacion</td><td bgcolor="#6A5ACD" color="white">)" +
                         ((string) buffer) + "</td></tr>\n").c_str());

                fprintf(archivoReporte, "%s", (R"(<tr><td color="white">mbr_disk_signature</td><td color="white">)" +
                                               to_string(mbr.mbr_dsk_signature) + "</td></tr>\n").c_str());

                for (int i = 0; i < 4; i++) {
                    if (mbr.mbr_partition_[i].part_start != -1) {
                        this->crearParticionRMBR(archivoReporte, archivoDisco, mbr.mbr_partition_[i]);
                    }
                }

                fprintf(archivoReporte, "</table>>];\n");
                fprintf(archivoReporte, "}");

                fclose(archivoDisco);
                fclose(archivoReporte);

                comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                          "/reporteMBR.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                system(comando.c_str());
                cout << "Reporte de MBR generado con Exito" << endl << endl;
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Disk
void Reporte::reporteDisk() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                FILE *archivoReporte = fopen((this->fichero_p + "/reporteDisk.dot").c_str(), "w+");

                fprintf(archivoReporte, "digraph G {\n");
                fprintf(archivoReporte, "node[shape=none]\n");
                fprintf(archivoReporte, "start[label=<<table><tr>");
                fprintf(archivoReporte, "<td rowspan=\"2\">MBR</td>");

                int tamanioT = mbr.mbr_tamano;
                int inicio = sizeof(MBR);
                for (int i = 0; i < 4; i++) {
                    if (mbr.mbr_partition_[i].part_start != -1) {
                        if (mbr.mbr_partition_[i].part_type == 'P') {
                            float p1 = (mbr.mbr_partition_[i].part_s * 1.0) / tamanioT;
                            float porcentaje = p1 * 100.0;
                            string name1 = mbr.mbr_partition_[i].part_name;
                            fprintf(archivoReporte, "<td rowspan=\"2\">%s <br/>%.2f %</td>", name1.c_str(), porcentaje);
                            if (i != 3) {
                                int aux = i;
                                for (i = i + 1; i < 4; i++) {
                                    if (mbr.mbr_partition_[i].part_start != -1) {
                                        if ((mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) <
                                            mbr.mbr_partition_[i].part_start) {
                                            porcentaje = ((mbr.mbr_partition_[i].part_start -
                                                           (mbr.mbr_partition_[aux].part_start +
                                                            mbr.mbr_partition_[aux].part_s) * 1.0) / tamanioT) * 100;
                                            fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>",
                                                    porcentaje);
                                            i = i - 1;
                                            break;
                                        } else if (
                                                (mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) ==
                                                mbr.mbr_partition_[i].part_start) {
                                            i = i - 1;
                                            break;
                                        }
                                    }
                                }
                                if (i == 4) {
                                    porcentaje =
                                            ((tamanioT -
                                              (mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) *
                                              1.0) /
                                             tamanioT);
                                    porcentaje = porcentaje * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                                    goto t0;
                                }
                            } else if ((mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s) < tamanioT) {
                                porcentaje = ((tamanioT -
                                               (mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s) *
                                               1.0) / tamanioT) * 100;
                                fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                            }

                        } else if (mbr.mbr_partition_[i].part_type == 'E') {
                            float porcentaje = ((mbr.mbr_partition_[i].part_s) * 1.0 / tamanioT) * 100.0;
                            fprintf(archivoReporte, "<td rowspan=\"2\">EXTENDIDA</td>");
                            EBR ebr, ebrAux;
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&ebr, sizeof(EBR), 1, archivoDisco);
                            if (!(ebr.part_s == 0 && ebr.part_next == -1)) {
                                string name1 = ebr.part_name;
                                fprintf(archivoReporte, ("<td rowspan=\"2\">EBR <br/>" + name1 + "</td>").c_str());
                                porcentaje = ((ebr.part_s * 1.0) / tamanioT) * 100.0;
                                fprintf(archivoReporte, "<td rowspan=\"2\">Logica <br/>%.2f %</td>", porcentaje);

                                int finAnterior;
                                while (ebr.part_next != -1) {
                                    fseek(archivoDisco, ebr.part_next, SEEK_SET);
                                    fread(&ebrAux, sizeof(EBR), 1, archivoDisco);

                                    int finAnterior = ebr.part_s + ebr.part_start;
                                    if (ebrAux.part_start > finAnterior) {
                                        porcentaje = ((ebrAux.part_start * 1.0 - finAnterior) / tamanioT) * 100.0;
                                        fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                                    }

                                    name1 = ebrAux.part_name;
                                    fprintf(archivoReporte, ("<td rowspan=\"2\">EBR <br/>" + name1 + "</td>").c_str());
                                    porcentaje = ((ebrAux.part_s * 1.0) / tamanioT) * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">Logica <br/>%.2f %</td>", porcentaje);
                                    ebr = ebrAux;
                                }

                                finAnterior = ebr.part_s + ebr.part_start;
                                if (mbr.mbr_partition_[i].part_s > finAnterior) {
                                    porcentaje =
                                            ((mbr.mbr_partition_[i].part_s * 1.0 - finAnterior) / tamanioT) * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                                }
                            } else {
                                fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                            }
                            fprintf(archivoReporte, "<td rowspan=\"2\">FIN EXTENDIDA</td>");
                        }
                        inicio = mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s;
                    } else {
                        for (i = i + 1; i < 4; i++) {
                            if (mbr.mbr_partition_[i].part_start != -1) {
                                float porcentaje =
                                        ((mbr.mbr_partition_[i].part_start * 1.0 - inicio) / tamanioT) * 100.0;
                                fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                                i = i - 1;
                                break;
                            }
                        }
                        if (i == 4) {
                            float porcentaje = (((tamanioT - inicio) * 1.0) / tamanioT) * (100.0);
                            fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                            goto t0;
                        }
                    }
                }
                t0:
                fprintf(archivoReporte, "</tr></table>>];\n");
                fprintf(archivoReporte, "}");

                fclose(archivoDisco);
                fclose(archivoReporte);

                comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                          "/reporteDisk.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                system(comando.c_str());
                cout << "Reporte de DISK generado con Exito" << endl << endl;
            }   else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Sb
void Reporte::reporteSb() {
    if (this->fichero_p != "" && this->archivo_p != "" &&
        verificarRuta(this->fichero_p, this->archivo_p, "[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")) {
        NodoMount *nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if(archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for(int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if(mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        }else if(mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteSb.dot").c_str(), "w+");
                    fprintf(archivoReporte, "digraph G {\n");
                    fprintf(archivoReporte, "node[shape=none]\n");
                    fprintf(archivoReporte, "start[label=<<table>\n");
                    fprintf(archivoReporte,
                            "<tr><td colspan=\"2\" bgcolor=\"#145a32\"><font color=\"white\">REPORTE DE SUPERBLOQUE</font></td></tr>\n");

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">sb_nombre_hd</td><td color=\"white\">" + nodo->nombre_disco +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_filesystem_type</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_filesystem_type) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inodes_count</td><td color=\"white\">" + to_string(sb.s_inodes_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_blocks_count</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_blocks_count) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_free_blocks_count</td><td color=\"white\">" + to_string(sb.s_free_blocks_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_free_inodes_count</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_free_inodes_count) + "</td></tr>\n").c_str());

                    tm *fecha = localtime(&sb.s_mtime);
                    char buffer[128];
                    strftime(buffer, sizeof(buffer), "%m-%d-%Y %X", fecha);
                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_mtime</td><td color=\"white\">" +
                             ((string) buffer) + "</td></tr>\n").c_str());

                    fecha = localtime(&sb.s_umtime);
                    char buffer2[128];
                    strftime(buffer2, sizeof(buffer2), "%m-%d-%Y %X", fecha);
                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_umtime</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             ((string) buffer2) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_mnt_count</td><td color=\"white\">" + to_string(sb.s_mnt_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_magic</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_magic) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inode_size</td><td color=\"white\">" + to_string(sb.s_inode_size) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_block_size</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_block_size) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_first_ino</td><td color=\"white\">" + to_string(sb.s_first_ino) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_first_blo</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_first_blo) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_bm_inode_start</td><td color=\"white\">" + to_string(sb.s_bm_inode_start) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_bm_block_start</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_bm_block_start) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inode_start</td><td color=\"white\">" + to_string(sb.s_inode_start) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_block_start</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_block_start) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte, "</table>>];\n");
                    fprintf(archivoReporte, "}");

                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteSb.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de SB generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Bm Inode
void Reporte::reporteBmInode() {
    if (this->fichero_p != "" && this->archivo_p != "" &&
        verificarRuta(this->fichero_p, this->archivo_p, "[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")) {
        NodoMount *nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if(archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for(int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if(mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        }else if(mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/"+this->archivo_p+".txt").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_inode_start,SEEK_SET);
                    int aux = 0;
                    for(int i = 0; i < sb.s_inodes_count; i++){
                        char caracter;
                        fread(&caracter, sizeof(caracter), 1, archivoDisco);
                        string caracterS = "";
                        caracterS += caracter;
                        fprintf(archivoReporte,"%s  ", caracterS.c_str());
                        aux++;
                        if(aux == 20){
                            fprintf(archivoReporte,"\n");
                            aux = 0;
                        }
                    }

                    fclose(archivoReporte);
                    cout << "Reporte de BM Inode generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Bm Inode
void Reporte::reporteBmBlock() {
    if (this->fichero_p != "" && this->archivo_p != "" &&
        verificarRuta(this->fichero_p, this->archivo_p, "[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")) {
        NodoMount *nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if(archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for(int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if(mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        }else if(mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/"+this->archivo_p+".txt").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_block_start,SEEK_SET);
                    int aux = 0;
                    for(int i = 0; i < sb.s_blocks_count; i++){
                        char caracter;
                        fread(&caracter, sizeof(caracter), 1, archivoDisco);
                        string caracterS = "";
                        caracterS += caracter;
                        fprintf(archivoReporte,"%s  ", caracterS.c_str());
                        aux++;
                        if(aux == 20){
                            fprintf(archivoReporte,"\n");
                            aux = 0;
                        }
                    }

                    fclose(archivoReporte);
                    cout << "Reporte de BM Blocks generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Inode
void Reporte::reporteInode() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for (int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if (mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        } else if (mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    char fecha[70];
                    string fecha2 = "";
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteInodo.dot").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_block_start,SEEK_SET);

                    fprintf(archivoReporte,"digraph G {\n");
                    fprintf(archivoReporte, "node[shape=none]\n");

                    for (int i = 0; i < sb.s_inodes_count; i++) {
                        char caracter;
                        fseek(archivoDisco, sb.s_bm_inode_start + i, SEEK_SET);
                        fread(&caracter, sizeof(char),1,archivoDisco);
                        if(caracter == '1'){
                            TablaInodo ti;
                            fseek(archivoDisco, sb.s_inode_start + (i * sizeof(TablaInodo)), SEEK_SET);
                            fread(&ti, sizeof(TablaInodo),1,archivoDisco);

                            fprintf(archivoReporte,("n"+ to_string(i)+
                                                    "[label=<<table><tr><td colspan=\"2\">INODO"+ to_string(i)+
                                                    "</td></tr>\n").c_str());

                            fprintf(archivoReporte,"<tr>\n<td>i_uid</td>\n");
                            fprintf(archivoReporte,("<td>"+ to_string(ti.i_uid)+"</td>\n").c_str());
                            fprintf(archivoReporte,"</tr>\n<tr>\n");
                            fprintf(archivoReporte,"<td>i_gid</td>\n");
                            fprintf(archivoReporte,("<td>"+ to_string(ti.i_gid)+"</td>\n").c_str());
                            fprintf(archivoReporte,"</tr>\n<tr>\n");
                            fprintf(archivoReporte,"<td>i_s</td>\n");
                            fprintf(archivoReporte,("<td>"+ to_string(ti.i_s)+"</td>\n").c_str());
                            fprintf(archivoReporte,"</tr>\n");

                            strftime(fecha,sizeof (fecha),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_atime));
                            fecha2=fecha;
                            fprintf(archivoReporte,"<tr>\n<td>i_atime</td>\n");
                            fprintf(archivoReporte,("<td>"+fecha2+"</td>\n</tr>\n").c_str());

                            strftime(fecha,sizeof (fecha),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_ctime));
                            fecha2=fecha;
                            fprintf(archivoReporte,"<tr>\n<td>i_ctime</td>\n");
                            fprintf(archivoReporte,("<td>"+fecha2+"</td>\n</tr>\n").c_str());

                            strftime(fecha,sizeof (fecha),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_mtime));
                            fecha2=fecha;
                            fprintf(archivoReporte,"<tr>\n<td>i_mtime</td>\n");
                            fprintf(archivoReporte,("<td>"+fecha2+"</td>\n</tr>\n").c_str());

                            for (int j = 0; j < 15; ++j) {
                                fprintf(archivoReporte,"<tr>\n<td>i_block</td>\n");
                                fprintf(archivoReporte,("<td>"+ to_string(ti.i_block[j])+"</td>\n</tr>\n").c_str());
                            }

                            fprintf(archivoReporte,"<tr>\n<td>i_type</td>\n");
                            fprintf(archivoReporte,("<td>"+ string(1,ti.i_type)+"</td>\n</tr>\n").c_str());

                            fprintf(archivoReporte,"<tr>\n<td>i_perm</td>\n");
                            fprintf(archivoReporte,("<td>"+ to_string(ti.i_perm)+"</td>\n</tr>\n").c_str());
                            fprintf(archivoReporte,"</table>>]\n");
                        }
                    }

                    fprintf(archivoReporte,"}");
                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteInodo.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de Inodo generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else {
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Block
void Reporte::reporteBlock() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for (int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if (mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        } else if (mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteBlock.dot").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_block_start,SEEK_SET);

                    fprintf(archivoReporte,"digraph G {\n");
                    fprintf(archivoReporte, "node[shape=none]\n");

                    for (int a = 0; a < sb.s_inodes_count; a++) {
                        char caracter;
                        TablaInodo ti;
                        BloqueApuntadores bap,bap1,bap2;

                        fseek(archivoDisco, sb.s_bm_inode_start + a, SEEK_SET);
                        fread(&caracter, sizeof(char),1,archivoDisco);
                        if(caracter == '1'){
                            fseek(archivoDisco, sb.s_inode_start + (a * sizeof(TablaInodo)), SEEK_SET);
                            fread(&ti, sizeof(TablaInodo),1,archivoDisco);

                            for (int i = 0; i < 15; ++i) {
                                if (ti.i_block[i]!=-1){
                                    if (i<12){
                                        if (ti.i_type=='0'){
                                            this->blockCarpeta(ti.i_block[i],archivoDisco,archivoReporte);
                                        }else if (ti.i_type=='1'){
                                            this->blockArchivo(ti.i_block[i],archivoDisco,archivoReporte);
                                        }
                                    } else if (i==12){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->blockApuntador(ti.i_block[i],archivoDisco,archivoReporte);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                if (ti.i_type=='0'){
                                                    this->blockCarpeta(bap.b_pointers[j],archivoDisco,archivoReporte);
                                                }else if (ti.i_type=='1'){
                                                    this->blockArchivo(bap.b_pointers[j],archivoDisco,archivoReporte);
                                                }
                                            }
                                        }
                                    }else if (i==13){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->blockApuntador(ti.i_block[i],archivoDisco,archivoReporte);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                fseek(archivoDisco,bap.b_pointers[j],SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores),1,archivoDisco);
                                                this->blockApuntador(bap.b_pointers[j],archivoDisco,archivoReporte);
                                                for (int k = 0; k < 16; ++k) {
                                                    if (bap1.b_pointers[k]!=-1){
                                                        if (ti.i_type=='0'){
                                                            this->blockCarpeta(bap1.b_pointers[k],archivoDisco,archivoReporte);
                                                        }else if (ti.i_type=='1'){
                                                            this->blockArchivo(bap1.b_pointers[k],archivoDisco,archivoReporte);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }else if (i==14){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->blockApuntador(ti.i_block[i],archivoDisco,archivoReporte);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                fseek(archivoDisco,bap.b_pointers[j],SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores),1,archivoDisco);
                                                this->blockApuntador(bap.b_pointers[j],archivoDisco,archivoReporte);
                                                for (int k = 0; k < 16; ++k) {
                                                    if (bap1.b_pointers[k]!=-1){
                                                        fseek(archivoDisco,bap1.b_pointers[k],SEEK_SET);
                                                        fread(&bap2, sizeof(BloqueApuntadores),1,archivoDisco);
                                                        this->blockApuntador(bap1.b_pointers[k],archivoDisco,archivoReporte);
                                                        for (int l = 0; l < 16; ++l) {
                                                            if (bap2.b_pointers[l]!=-1){
                                                                if (ti.i_type=='0'){
                                                                    this->blockCarpeta(bap2.b_pointers[l],archivoDisco,archivoReporte);
                                                                }else if (ti.i_type=='1'){
                                                                    this->blockArchivo(bap2.b_pointers[l],archivoDisco,archivoReporte);
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

                    fprintf(archivoReporte,"}");
                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteBlock.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de Block generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else {
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

void Reporte::blockCarpeta(int posicion, FILE *archivoDisco, FILE *archivoReporte) {
    BloqueCarpeta carpeta;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&carpeta, sizeof(BloqueCarpeta),1,archivoDisco);

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td colspan=\"2\" bgcolor=\"#c3f8b6\">Bloque Carpeta</td>\n");
    fprintf(archivoReporte,"</tr>\n");

    for (int i = 0; i < 4; ++i) {
        string b_name="";
        for (int j = 0; j < 12; ++j) {
            if (carpeta.b_content[i].b_name[j]=='\000'){
                break;
            }
            b_name+=carpeta.b_content[i].b_name[j];
        }

        fprintf(archivoReporte,"<tr>\n");
        fprintf(archivoReporte,("<td colspan=\"2\" bgcolor=\"#b6f8d3\">b_content "+ to_string(i)+"</td>\n").c_str());
        fprintf(archivoReporte,"</tr>\n<tr>\n");
        fprintf(archivoReporte,"<td>b_name</td>\n");
        fprintf(archivoReporte,("<td>"+b_name+"</td>\n").c_str());
        fprintf(archivoReporte,"</tr>\n<tr>\n");
        fprintf(archivoReporte,"<td>b_inodo</td>\n");
        fprintf(archivoReporte,("<td>"+ to_string(carpeta.b_content[i].b_inodo)+"</td>\n").c_str());
        fprintf(archivoReporte,"</tr>\n");
    }
    fprintf(archivoReporte,"</table>>]\n");
}

void Reporte::blockArchivo(int posicion, FILE *archivoDisco, FILE *archivoReporte) {
    string contenido = "";
    BloqueArchivo archivo;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&archivo, sizeof(BloqueArchivo),1,archivoDisco);

    for (int i = 0; i < 64; ++i) {
        if (archivo.b_content[i]=='\000'){
            break;
        }
        contenido+=archivo.b_content[i];
    }

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td bgcolor=\"#c3f8b6\">Bloque Archivo</td>");
    fprintf(archivoReporte,"</tr>\n<tr>\n");
    fprintf(archivoReporte,("<td>"+contenido+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n</table>>]\n");
}

void Reporte::blockApuntador(int posicion, FILE *archivoDisco, FILE *archivoReporte) {
    BloqueApuntadores apuntador;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&apuntador, sizeof(BloqueApuntadores),1,archivoDisco);

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td colspan=\"2\" bgcolor=\"#c3f8b6\">Bloque Apuntadores</td>");
    fprintf(archivoReporte,"</tr>\n");
    for (int i = 0; i < 16; ++i) {
        fprintf(archivoReporte,"<tr>\n");
        fprintf(archivoReporte,("<td>b_pointer "+ to_string(i)+"</td>\n").c_str());
        fprintf(archivoReporte,("<td>"+ to_string(apuntador.b_pointers[i])+"</td>\n").c_str());
        fprintf(archivoReporte,"</tr>\n");
    }
    fprintf(archivoReporte,"</table>>]\n");
}

//Generar Reporte File
void Reporte::reporteFile() {
    if(verificarRuta(this->fichero_r,this->archivo_r,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+") && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                int inicioSB = -1;
                for (int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            inicioSB = mbr.mbr_partition_[i].part_start;
                            break;
                        } else if (mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        } else if (mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                inicioSB = ebr.part_start + sizeof(EBR);
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(inicioSB != -1){
                    char fecha[70];
                    string fecha2="";
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteFile.dot").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_block_start,SEEK_SET);

                    fprintf(archivoReporte,"digraph G {\n");
                    fprintf(archivoReporte, "node[shape=none]\n");


                    vector<string> ficheros = this->split(this->fichero_r,'/');
                    if(this->archivo_r != ""){
                        ficheros.push_back(archivo_r);
                    }

                    int ubicacion = this->buscarFichero(ficheros,sb,inicioSB,sb.s_inode_start,archivoDisco);

                    fprintf(archivoReporte,"start[label=<<table>\n");

                    fprintf(archivoReporte,"<tr>\n");
                    fprintf(archivoReporte,"<td>Contenido</td>\n");
                    fprintf(archivoReporte,"</tr>\n");

                    if(ubicacion != -1) {
                        fprintf(archivoReporte,"<tr>\n<td>\n");
                        fprintf(archivoReporte, this->getContentF(ubicacion, archivoDisco).c_str());
                        fprintf(archivoReporte,"\n</td>\n</tr>\n");
                    }
                    fprintf(archivoReporte,"</table>>]");


                    fprintf(archivoReporte,"}");
                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteFile.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de File generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else {
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Tree
void Reporte::reporteTree() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                SuperBloque sb;
                bool verificar = false;
                for (int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if (mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        } else if (mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteTree.dot").c_str(), "w+");
                    fseek(archivoDisco, sb.s_bm_block_start,SEEK_SET);
                    string conexiones = "";

                    fprintf(archivoReporte,"digraph G {\n");
                    fprintf(archivoReporte, "rankdir=LR;\n");
                    fprintf(archivoReporte, "node[shape=none]\n");

                    for (int a = 0; a < sb.s_inodes_count; a++) {
                        char caracter;
                        TablaInodo ti;
                        BloqueApuntadores bap,bap1,bap2;

                        fseek(archivoDisco, sb.s_bm_inode_start + a, SEEK_SET);
                        fread(&caracter, sizeof(char),1,archivoDisco);
                        if(caracter == '1'){
                            fseek(archivoDisco, sb.s_inode_start + (a * sizeof(TablaInodo)), SEEK_SET);
                            fread(&ti, sizeof(TablaInodo),1,archivoDisco);

                            this->treeInodo(sb.s_inode_start + (a * sizeof(TablaInodo)),a,archivoDisco,archivoReporte,conexiones);
                            for (int i = 0; i < 15; ++i) {
                                if (ti.i_block[i]!=-1){
                                    if (i<12){
                                        if (ti.i_type=='0'){
                                            this->treeCarpeta(ti.i_block[i],archivoDisco,archivoReporte,conexiones);
                                        }else if (ti.i_type=='1'){
                                            this->treeArchivo(ti.i_block[i],archivoDisco,archivoReporte,conexiones);
                                        }
                                    } else if (i==12){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->treeApuntador(ti.i_block[i],archivoDisco,archivoReporte,conexiones);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                if (ti.i_type=='0'){
                                                    this->treeCarpeta(bap.b_pointers[j],archivoDisco,archivoReporte,conexiones);
                                                }else if (ti.i_type=='1'){
                                                    this->treeArchivo(bap.b_pointers[j],archivoDisco,archivoReporte,conexiones);
                                                }
                                            }
                                        }
                                    }else if (i==13){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->treeApuntador(ti.i_block[i],archivoDisco,archivoReporte,conexiones);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                fseek(archivoDisco,bap.b_pointers[j],SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores),1,archivoDisco);
                                                this->treeApuntador(bap.b_pointers[j],archivoDisco,archivoReporte,conexiones);
                                                for (int k = 0; k < 16; ++k) {
                                                    if (bap1.b_pointers[k]!=-1){
                                                        if (ti.i_type=='0'){
                                                            this->treeCarpeta(bap1.b_pointers[k],archivoDisco,archivoReporte,conexiones);
                                                        }else if (ti.i_type=='1'){
                                                            this->treeArchivo(bap1.b_pointers[k],archivoDisco,archivoReporte,conexiones);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }else if (i==14){
                                        fseek(archivoDisco,ti.i_block[i],SEEK_SET);
                                        fread(&bap, sizeof(BloqueApuntadores),1,archivoDisco);
                                        this->treeApuntador(ti.i_block[i],archivoDisco,archivoReporte,conexiones);
                                        for (int j = 0; j < 16; ++j) {
                                            if (bap.b_pointers[j]!=-1){
                                                fseek(archivoDisco,bap.b_pointers[j],SEEK_SET);
                                                fread(&bap1, sizeof(BloqueApuntadores),1,archivoDisco);
                                                this->treeApuntador(bap.b_pointers[j],archivoDisco,archivoReporte,conexiones);
                                                for (int k = 0; k < 16; ++k) {
                                                    if (bap1.b_pointers[k]!=-1){
                                                        fseek(archivoDisco,bap1.b_pointers[k],SEEK_SET);
                                                        fread(&bap2, sizeof(BloqueApuntadores),1,archivoDisco);
                                                        this->treeApuntador(bap1.b_pointers[k],archivoDisco,archivoReporte,conexiones);
                                                        for (int l = 0; l < 16; ++l) {
                                                            if (bap2.b_pointers[l]!=-1){
                                                                if (ti.i_type=='0'){
                                                                    this->treeCarpeta(bap2.b_pointers[l],archivoDisco,archivoReporte,conexiones);
                                                                }else if (ti.i_type=='1'){
                                                                    this->treeArchivo(bap2.b_pointers[l],archivoDisco,archivoReporte,conexiones);
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
                    fprintf(archivoReporte,conexiones.c_str());
                    fprintf(archivoReporte,"}");
                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteTree.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de Tree generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else {
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

void Reporte::treeInodo(int posicion, int noInodo, FILE *archivoDisco, FILE *archivoReporte, string & conexiones) {
    string dot="";
    char buffer[128];
    string fecha="";

    TablaInodo ti;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&ti, sizeof(TablaInodo),1,archivoDisco);

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table><tr><td colspan=\"2\" bgcolor=\"#376ef3\">INODO "+ to_string(noInodo)+"</td></tr>\n").c_str());

    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_uid</td>\n");
    fprintf(archivoReporte,("<td>"+ to_string(ti.i_uid)+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_gid</td>\n");
    fprintf(archivoReporte,("<td>"+ to_string(ti.i_gid)+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_s</td>\n");
    fprintf(archivoReporte,("<td>"+ to_string(ti.i_s)+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    strftime(buffer,sizeof (buffer),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_atime));
    fecha=buffer;
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_atime</td>\n");
    fprintf(archivoReporte,("<td>"+fecha+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    strftime(buffer,sizeof (buffer),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_ctime));
    fecha=buffer;
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_ctime</td>\n");
    fprintf(archivoReporte,("<td>"+fecha+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    strftime(buffer,sizeof (buffer),"%Y-%m-%d %H:%M:%S",localtime(&ti.i_mtime));
    fecha=buffer;
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_mtime</td>\n");
    fprintf(archivoReporte,("<td>"+fecha+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    for (int j = 0; j < 15; ++j) {
        if (ti.i_block[j]!=-1){
            conexiones += "n"+to_string(posicion)+" -> n"+ to_string(ti.i_block[j]) +"\n";
            fprintf(archivoReporte,"<tr>\n");
            fprintf(archivoReporte,("<td>ap"+ to_string(j)+"</td>\n").c_str());
            fprintf(archivoReporte,("<td port=\""+to_string(ti.i_block[j])+"\">"+ to_string(ti.i_block[j])+"</td>\n").c_str());
            fprintf(archivoReporte,"</tr>\n");
        }else{
            fprintf(archivoReporte,"<tr>\n");
            fprintf(archivoReporte,"<td>i_block</td>\n");
            fprintf(archivoReporte,"<td>-1</td>\n");
            fprintf(archivoReporte,"</tr>\n");
        }

    }

    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_type</td>\n");
    fprintf(archivoReporte,("<td>"+ string(1,ti.i_type)+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td>i_perm</td>\n");
    fprintf(archivoReporte,("<td>"+ to_string(ti.i_perm)+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n");

    fprintf(archivoReporte,"</table>>]\n");
}

void Reporte::treeApuntador(int posicion, FILE *archivoDisco, FILE *archivoReporte, string &conexiones) {
    BloqueApuntadores apuntador;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&apuntador, sizeof(BloqueApuntadores),1,archivoDisco);

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td colspan=\"2\" bgcolor=\"#c3f8b6\">Bloque Apuntadores</td>");
    fprintf(archivoReporte,"</tr>\n");
    for (int i = 0; i < 16; ++i) {
        fprintf(archivoReporte,"<tr>\n");
        fprintf(archivoReporte,("<td>b_pointer "+ to_string(i)+"</td>\n").c_str());
        fprintf(archivoReporte,("<td>"+ to_string(apuntador.b_pointers[i])+"</td>\n").c_str());
        if(apuntador.b_pointers[i] != -1){
            conexiones += "n"+to_string(posicion)+" -> n"+ to_string(apuntador.b_pointers[i]) +"\n";
        }
        fprintf(archivoReporte,"</tr>\n");
    }
    fprintf(archivoReporte,"</table>>]\n");
}

void Reporte::treeCarpeta(int posicion, FILE *archivoDisco, FILE *archivoReporte, string &conexiones) {
    BloqueCarpeta carpeta;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&carpeta, sizeof(BloqueCarpeta),1,archivoDisco);

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td colspan=\"2\" bgcolor=\"#f34037\">Bloque Carpeta</td>");
    fprintf(archivoReporte,"</tr>\n");
    for (int i = 0; i < 4; ++i) {
        string b_name="";
        for (int j = 0; j < 12; ++j) {
            if (carpeta.b_content[i].b_name[j]=='\000'){
                break;
            }
            b_name+=carpeta.b_content[i].b_name[j];
        }

        if(carpeta.b_content[i].b_inodo != -1){
            conexiones += "n"+to_string(posicion)+" -> n"+ to_string(carpeta.b_content[i].b_inodo) +"\n";
        }

        fprintf(archivoReporte,"<tr>\n");
        fprintf(archivoReporte,("<td>"+b_name+"</td>\n").c_str());
        fprintf(archivoReporte,("<td port=\""+ to_string(carpeta.b_content[i].b_inodo)+"\">"+ to_string(carpeta.b_content[i].b_inodo)+"</td>\n").c_str());
        fprintf(archivoReporte,"</tr>\n");
    }
    fprintf(archivoReporte,"</table>>]\n");
}

void Reporte::treeArchivo(int posicion, FILE *archivoDisco, FILE *archivoReporte, string &conexiones) {
    string content="";
    BloqueArchivo archivo;
    fseek(archivoDisco,posicion,SEEK_SET);
    fread(&archivo, sizeof(BloqueArchivo),1,archivoDisco);

    for (int i = 0; i < 64; ++i) {
        if (archivo.b_content[i]=='\000'){
            break;
        }
        content+=archivo.b_content[i];
    }

    fprintf(archivoReporte,("n"+ to_string(posicion)+"[label=<<table>\n").c_str());
    fprintf(archivoReporte,"<tr>\n");
    fprintf(archivoReporte,"<td colspan=\"2\" bgcolor=\"#c3f8b6\">Bloque Archivo</td>");
    fprintf(archivoReporte,"</tr>\n<tr>\n");
    fprintf(archivoReporte,("<td>"+content+"</td>\n").c_str());
    fprintf(archivoReporte,"</tr>\n</table>>]\n");
}

//Split string
vector<string> Reporte::split(string cadena,char delim) {
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

//Obtener la informacion de un archivo en disco
string Reporte::getContentF(int inicioInodo, FILE *archivo) {
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

int Reporte::buscarFichero(vector<string> &ficheros, SuperBloque &sb, int inicioSB, int inicioInodo, FILE *archivo) {
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
                return this->buscarFichero(ficheros,sb,inicioSB,ubicacion,archivo);

            } else { cout << "No se encontro el fichero " << fichero << endl << endl; return -1; }
        } else { cout << "El inodo no corresponde a una carpeta" << endl << endl; return -1; }
    }
    else {
        return inicioInodo;
    }
}

//Buscar una carpeta o archivo en una carpeta padre
int Reporte::buscarEnCarpeta(TablaInodo &ti, int inicioInodo, FILE *archivo, string nombre) {
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